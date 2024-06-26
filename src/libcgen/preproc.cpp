#include <debug.hpp>
#include <iterator>
#include <poost/assert.hpp>
#include <preproc.hpp>
#include <string_view>
#include <utility>

namespace cgen {

auto node_clone(const YAML::Node &node) -> YAML::Node;
auto node_find(const YAML::Node &map,
               const std::string &key) -> std::pair<YAML::Node, std::string>;

auto string_key_attribute(const std::string &key)
    -> std::pair<std::string, std::string_view>;
auto string_replace_parameters(
    const std::string &str,
    const std::map<std::string, std::string, std::less<void>> &params,
    std::vector<std::string> &undefined_params) -> std::string;

auto node_is_defined(const YAML::Node &node, const std::string &key) -> bool;
void node_wrap_configs(YAML::Node node);
void node_wrap_visibility(YAML::Node node);

/// Public

void node_merge(const YAML::Node &from_node, YAML::Node &to_node) {
    if (!from_node.IsDefined() || from_node.IsNull()) {
        return;
    }

    if (to_node.IsDefined() && from_node.IsMap() && to_node.IsMap()) {
        // merge if both nodes are maps
        for (const auto &it : from_node) {
            const YAML::Node from_key_node = it.first;
            const YAML::Node from_val_node = it.second;
            const std::string from_key = from_key_node.as<std::string>();
            const std::pair<std::string, std::string_view> from_key_attr_pair =
                string_key_attribute(from_key);
            const std::pair<YAML::Node, std::string> to_node_attr_pair =
                node_find(to_node, from_key_attr_pair.first);

            if (to_node_attr_pair.second == "REPLACE") {
                // do nothing, keep original node with original attribute
            } else if (from_key_attr_pair.second == "REPLACE") {
                // replace original node without merging
                to_node[from_key_attr_pair.first] = node_clone(from_val_node);
            } else {
                // merge
                YAML::Node to_val_node = to_node_attr_pair.first.IsDefined()
                                             ? to_node_attr_pair.first
                                             : YAML::Node();
                node_merge(from_val_node, to_val_node);
                to_node[from_key_attr_pair.first] = to_val_node;
            }
        }
    } else if (to_node.IsDefined() && from_node.IsSequence() &&
               to_node.IsSequence()) {

        // append if both nodes are lists
        for (const auto &it : from_node) {
            to_node.push_back(node_clone(it));
        }
    } else {
        // replace otherwise
        to_node = node_clone(from_node);
    }
}

void node_replace_parameters(
    YAML::Node &node,
    const std::map<std::string, std::string, std::less<void>> &params,
    std::vector<std::string> &undefined_params) {

    if (node.IsMap()) {
        for (auto it : node) {
            node_replace_parameters(it.second, params, undefined_params);
        }
    } else if (node.IsSequence()) {
        for (auto it : node) {
            node_replace_parameters(it, params, undefined_params);
        }
    } else if (node.IsScalar()) {
        const std::string val = node.as<std::string>();
        node = string_replace_parameters(val, params, undefined_params);
    }
}

void node_wrap_configs(const YAML::Node &node, const std::string &key) {
    node_wrap_configs(node[key]);
    node_wrap_configs(node[key + ":REPLACE"]);
}

void node_wrap_visibility(const YAML::Node &node, const std::string &key) {
    node_wrap_visibility(node[key]);
    node_wrap_visibility(node[key + ":REPLACE"]);
}

/// Private

void node_trim_attributes(YAML::Node &node) {
    if (node.IsMap()) {
        for (auto it : node) {
            YAML::Node key_node = it.first;
            YAML::Node val_node = it.second;
            const std::string key = key_node.as<std::string>();
            const std::pair<std::string, std::string_view> key_attr_pair =
                string_key_attribute(key);
            key_node = key_attr_pair.first;
            node_trim_attributes(val_node);
        }
    }
}

auto node_clone(const YAML::Node &node) -> YAML::Node {
    if (node.IsMap()) {
        YAML::Node cloned_node{YAML::NodeType::Map};

        for (const auto &it : node) {
            const YAML::Node key_node = it.first;
            const YAML::Node val_node = it.second;
            const std::string key = key_node.as<std::string>();
            const std::pair<std::string, std::string_view> key_attr_pair =
                string_key_attribute(key);
            cloned_node[key_attr_pair.first] = node_clone(val_node);
        }

        return cloned_node;
    } else {
        return YAML::Clone(node);
    }
}

auto node_find(const YAML::Node &map,
               const std::string &key) -> std::pair<YAML::Node, std::string> {

    POOST_ASSERT(map.IsMap(), "node is not a map: {}", node_dump(map));

    const YAML::Node node = map[key];

    if (node.IsDefined()) {
        return std::pair<YAML::Node, std::string>(node, "");
    }

    for (const auto &it : map) {
        const YAML::Node key_node = it.first;
        const YAML::Node val_node = it.second;
        const std::string node_key = key_node.as<std::string>();
        const std::pair<std::string, std::string_view> node_key_attr_pair =
            string_key_attribute(node_key);

        if (node_key_attr_pair.first == key) {
            return std::pair<YAML::Node, std::string>(
                val_node, std::string{node_key_attr_pair.second});
        }
    }

    return std::pair<YAML::Node, std::string>(node, "");
}

auto string_key_attribute(const std::string &key)
    -> std::pair<std::string, std::string_view> {

    const std::string::size_type pos = key.find(':');

    if (pos != std::string::npos) {
        return std::pair<std::string, std::string_view>(
            key.substr(0, pos), std::string_view{key}.substr(pos + 1));
    } else {
        return std::pair<std::string, std::string_view>(key,
                                                        std::string_view{});
    }
}

auto string_replace_parameters(
    const std::string &str,
    const std::map<std::string, std::string, std::less<void>> &params,
    std::vector<std::string> &undefined_params) -> std::string {

    if (str.empty()) {
        return str;
    }

    enum {
        ST_NONE,
        ST_PARAM_BEGIN,
        ST_PARAM,
    } state = ST_NONE;

    std::string_view param;

    std::string result;
    result.reserve(str.size());

    for (auto it = str.cbegin(); it != str.cend(); ++it) {
        const bool is_last = std::next(it) == str.cend();
        const unsigned char c = *it;

        switch (state) {
        case ST_NONE:
            if (c == '$' && !is_last) {
                state = ST_PARAM_BEGIN;
            } else {
                result += c;
            }

            break;
        case ST_PARAM_BEGIN:
            if (c == '(') {
                state = ST_PARAM;
            } else if (c == '$') {
                state = ST_NONE;
                result += c;
            } else {
                state = ST_NONE;
                result += "$";
                result += c;
            }

            break;
        case ST_PARAM:
            if (c == ')') {
                const auto param_it = params.find(param);
                if (param_it != params.end()) {
                    result += param_it->second;
                } else {
                    undefined_params.push_back(std::string{param});
                }

                state = ST_NONE;
                param = std::string_view{};
            } else {
                param = std::string_view{
                    !param.empty() ? param.cbegin() : &(*it), param.size() + 1};
            }

            break;
        default:
            POOST_ASSERT_FAIL("invalid params replacer state: {}", state);
        }
    }

    POOST_ASSERT(state == ST_NONE, "invalid params replacer end state: {}",
                 state);

    return result;
}

auto node_is_defined(const YAML::Node &node, const std::string &key) -> bool {
    return node[key].IsDefined() || node[key + ":REPLACE"].IsDefined();
}

void node_wrap_configs(YAML::Node node) {
    if (!node.IsDefined()) {
        return;
    }

    if (node_is_defined(node, "global") ||
        node_is_defined(node, "configurations")) {
        return;
    }

    YAML::Node defaults;
    defaults["global"] = node;
    node = defaults;
}

void node_wrap_visibility(YAML::Node node) {
    if (!node.IsDefined()) {
        return;
    }

    if (node_is_defined(node, "default") || node_is_defined(node, "public") ||
        node_is_defined(node, "private") ||
        node_is_defined(node, "interface")) {

        node_wrap_configs(node, "default");
        node_wrap_configs(node, "public");
        node_wrap_configs(node, "private");
        node_wrap_configs(node, "interface");
    } else {
        YAML::Node defaults;
        node_wrap_configs(node);
        defaults["default"] = node;
        node = defaults;
    }
}

} // namespace cgen
