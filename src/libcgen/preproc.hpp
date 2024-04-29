#ifndef CGEN_PREPROC_HPP
#define CGEN_PREPROC_HPP

#include <yaml-cpp/yaml.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace cgen {

void node_merge(const YAML::Node &from_node, YAML::Node &to_node);
void node_replace_parameters(
    YAML::Node &node,
    const std::map<std::string, std::string, std::less<void>> &params,
    std::vector<std::string> &undefined_params);
void node_trim_attributes(YAML::Node &node);

void node_wrap_configs(const YAML::Node &node, const std::string &key);
void node_wrap_visibility(const YAML::Node &node, const std::string &key);

} // namespace cgen

#endif // ifndef CGEN_PREPROC_HPP
