#define MIROIR_IMPLEMENTATION
#define MIROIR_YAMLCPP_SPECIALIZATION

#include <yaml-cpp/yaml.h>
#include <config.hpp>
#include <fs.hpp>
#include <miroir/miroir.hpp>
#include <poost/log.hpp>
#include <preproc.hpp>
#include <set>

namespace YAML {

template <typename T>
auto as(const Node& node) -> T {
    return node.as<T>(T{});
}

template <typename T>
auto as(const Node& node, const T& fallback) -> T {
    return node.as<T>(fallback);
}

}  // namespace YAML

namespace cgen {

static auto node_check_version(const YAML::Node& config_node, int ver, std::vector<Error>& errors)
    -> bool;
static auto node_validate(const YAML::Node& config_node, std::vector<Error>& errors) -> bool;

static void node_merge_includes(YAML::Node& config_node,
                                std::set<std::string>& included_paths,
                                std::vector<Error>& errors);
static void node_merge_templates(const YAML::Node& config_node, std::vector<Error>& errors);

/// Public

auto config_read(std::istream& in, int ver, std::vector<Error>& errors) -> Config {
    // load
    YAML::Node config_node = YAML::Load(in);

    // check version
    POOST_TRACE("check config version");
    if (!node_check_version(config_node, ver, errors)) {
        return Config{};
    }

    // preprocess
    POOST_TRACE("preprocess config");
    {
        // validate before merging includes
        POOST_TRACE("validate config before merging includes");
        if (!node_validate(config_node, errors)) {
            return Config{};
        }

        std::set<std::string> included_paths;
        POOST_TRACE("merge includes");
        node_merge_includes(config_node, included_paths, errors);

        // validate before merging templates
        POOST_TRACE("validate config before merging templates");
        if (!node_validate(config_node, errors)) {
            return Config{};
        }

        POOST_TRACE("merge templates");
        node_merge_templates(config_node, errors);
        node_trim_attributes(config_node);
    }

    // validate after preprocessing
    POOST_TRACE("validate config after preprocessing");
    if (!node_validate(config_node, errors)) {
        return Config{};
    }

    if (!errors.empty()) {
        return Config{};
    }

    // parse
    POOST_TRACE("parse config");
    Config config = YAML::as<Config>(config_node);

    return config;
}

/// Validation

static auto node_check_version(const YAML::Node& config_node, int ver, std::vector<Error>& errors)
    -> bool {
    if (!config_node.IsDefined() || !config_node.IsMap() || !config_node["version"].IsDefined() ||
        !config_node["version"].IsScalar()) {
        return true;
    }

    const int config_ver = YAML::as<int>(config_node["version"]);
    if (config_ver == ver) {
        return true;
    }

    POOST_ERROR("unsupported config version: {}", config_ver);

    errors.push_back(Error{
        .type = ErrorType::ConfigUnsupportedVersion,
        .source = "",
        .subject = std::to_string(config_ver),
    });

    return false;
}

static auto make_validator() -> miroir::Validator<YAML::Node> {
    // embed schema in the source code
    const char* schema_yaml =
#include "cgen.schema.yml.in"
        ;
    const YAML::Node schema_node = YAML::Load(schema_yaml);
    const miroir::Validator<YAML::Node> validator = miroir::Validator<YAML::Node>(schema_node);
    return validator;
}

static auto node_validate(const YAML::Node& config_node, std::vector<Error>& errors) -> bool {
    static const miroir::Validator<YAML::Node> validator = make_validator();

    const std::vector<miroir::Error<YAML::Node>> validation_errors =
        validator.validate(config_node);

    if (!validation_errors.empty()) {
        for (const auto& validation_err : validation_errors) {
            const std::string desc = validation_err.description();
            POOST_ERROR("config validation error: {}", desc);

            errors.emplace_back(Error{
                .type = ErrorType::ConfigValidationError,
                .source = "",
                .subject = desc,
            });
        }

        return false;
    }

    return true;
}

/// Includes

static void node_merge_includes(YAML::Node& config_node,
                                std::set<std::string>& included_paths,
                                std::vector<Error>& errors) {
    Config config = YAML::as<Config>(config_node);

    std::vector<std::string> undefined_params;

    for (const config::Include& include : config.includes) {
        for (const std::string& include_path : include.paths) {
            if (!path_exists(include_path)) {
                POOST_ERROR("config include not found: {}", include_path);

                errors.push_back(Error{
                    .type = ErrorType::ConfigIncludeNotFound,
                    .source = "",
                    .subject = include_path,
                });

                continue;
            }

            if (included_paths.contains(include_path)) {
                continue;
            }

            included_paths.insert(include_path);

            std::istream is{nullptr};
            file_read(include_path, is);
            YAML::Node include_node = YAML::Load(is);

            undefined_params.clear();
            node_replace_parameters(include_node, include.parameters, undefined_params);

            node_merge_includes(include_node, included_paths, errors);

            // ignore some fields in included configs
            include_node.remove("version");
            include_node.remove("project");
            include_node.remove("includes");  // ignore nested includes just to
                                              // make things simpler
            node_merge(include_node, config_node);

            for (const std::string& param : undefined_params) {
                POOST_ERROR("undefined config include param: {}", param);

                errors.push_back(Error{
                    .type = ErrorType::ConfigUndefinedIncludeParameter,
                    .source = include_path,
                    .subject = param,
                });
            }
        }
    }
}

/// Templates

static void node_merge_templates(const YAML::Node& config_node, std::vector<Error>& errors) {
    Config config = YAML::as<Config>(config_node);

    std::vector<std::string> undefined_params;

    for (config::Target& target : config.targets) {
        for (const config::Template& tpl : target.templates) {
            for (const std::string& tpl_name : tpl.names) {
                const auto tpl_it = config.templates.find(tpl_name);

                if (tpl_it == config.templates.end()) {
                    POOST_ERROR("config template not found: {}", tpl_name);

                    errors.push_back(Error{
                        .type = ErrorType::ConfigTemplateNotFound,
                        .source = target.name,
                        .subject = tpl_name,
                    });

                    continue;
                }

                YAML::Node tpl_node = YAML::Clone(tpl_it->second.node);

                undefined_params.clear();
                node_replace_parameters(tpl_node, tpl.parameters, undefined_params);

                node_merge(tpl_node, target.node);

                for (const std::string& param : undefined_params) {
                    POOST_ERROR("undefined config template param: {}", param);

                    errors.push_back(Error{
                        .type = ErrorType::ConfigUndefinedTemplateParameter,
                        .source = tpl_name,
                        .subject = param,
                    });
                }
            }
        }

        node_trim_attributes(target.node);
    }
}

}  // namespace cgen

/// Decoders

namespace YAML {

template <>
struct convert<cgen::Config> {
    static auto decode(const Node& node, cgen::Config& config) -> bool {
        config.version = as<std::string>(node["version"]);
        config.project = as<cgen::config::Project>(node["project"]);
        config.includes = as<std::vector<cgen::config::Include>>(node["includes"]);
        config.templates =
            as<std::map<std::string, cgen::config::TargetSettings>>(node["templates"]);
        config.options = as<std::map<std::string, cgen::config::Option>>(node["options"]);
        config.settings = as<std::map<std::string, cgen::config::Expression>>(node["settings"]);
        config.packages = as<std::vector<cgen::config::Package>>(node["packages"]);
        config.targets = as<std::vector<cgen::config::Target>>(node["targets"]);
        return true;
    }
};

template <>
struct convert<cgen::config::Project> {
    static auto decode(const Node& node, cgen::config::Project& project) -> bool {
        if (node.IsScalar()) {
            project.name = as<std::string>(node);
        } else {
            project.name = as<std::string>(node["name"]);
            project.version = as<std::string>(node["version"]);
        }

        return true;
    }
};

template <>
struct convert<cgen::config::Include> {
    static auto decode(const Node& node, cgen::config::Include& include) -> bool {
        if (node.IsScalar()) {
            const std::string path = as<std::string>(node);
            include.paths.push_back(path);
        } else {
            include.paths = as<std::vector<std::string>>(node["paths"]);
            include.parameters =
                as<std::map<std::string, std::string, std::less<void>>>(node["parameters"]);
        }

        return true;
    }
};

template <>
struct convert<cgen::config::Option> {
    static auto decode(const Node& node, cgen::config::Option& opt) -> bool {
        opt.description = as<std::string>(node["description"]);
        opt.default_ = as<cgen::config::Expression>(node["default"]);
        return true;
    }
};

template <>
struct convert<cgen::config::Package> {
    static auto decode(const Node& node, cgen::config::Package& pkg) -> bool {
        pkg.if_ = as<std::string>(node["if"]);

        if (node["external"].IsDefined()) {
            pkg.type = cgen::config::PackageType::External;
            pkg.name = as<std::string>(node["external"]);
            pkg.external = as<cgen::config::ExternalPackage>(node);
            return true;
        }

        if (node["system"].IsDefined()) {
            pkg.type = cgen::config::PackageType::System;
            pkg.name = as<std::string>(node["system"]);
            pkg.system = as<cgen::config::SystemPackage>(node);
            return true;
        }

        return false;
    }
};

template <>
struct convert<cgen::config::FetchStrategy> {
    static auto decode(const Node& node, cgen::config::FetchStrategy& strategy) -> bool {
        const std::string strategy_str = as<std::string>(node);

        if (strategy_str == "submodule") {
            strategy = cgen::config::FetchStrategy::Submodule;
            return true;
        }

        if (strategy_str == "clone") {
            strategy = cgen::config::FetchStrategy::Clone;
            return true;
        }

        return false;
    }
};
template <>
struct convert<cgen::config::ExternalPackage> {
    static auto decode(const Node& node, cgen::config::ExternalPackage& pkg) -> bool {
        pkg.url = as<std::string>(node["url"]);
        pkg.version = as<std::string>(node["version"]);
        pkg.strategy = as<cgen::config::FetchStrategy>(node["strategy"],
                                                       cgen::config::FetchStrategy::Submodule);
        pkg.options = as<std::map<std::string, cgen::config::Expression>>(node["options"]);
        return true;
    }
};

template <>
struct convert<cgen::config::SystemPackage> {
    static auto decode(const Node& node, cgen::config::SystemPackage& pkg) -> bool {
        pkg.version = as<std::string>(node["version"]);
        pkg.is_required = as<bool>(node["required"], true);
        return true;
    }
};

template <>
struct convert<cgen::config::Target> {
    static auto decode(const Node& node, cgen::config::Target& target) -> bool {
        target.node = node;
        target.templates = as<std::vector<cgen::config::Template>>(node["templates"]);
        target.if_ = as<std::string>(node["if"]);

        if (node["library"].IsDefined()) {
            target.type = cgen::config::TargetType::Library;
            target.name = as<std::string>(node["library"]);
            target.library = as<cgen::config::LibraryTarget>(node);
            return true;
        }

        if (node["executable"].IsDefined()) {
            target.type = cgen::config::TargetType::Executable;
            target.name = as<std::string>(node["executable"]);
            target.executable = as<cgen::config::ExecutableTarget>(node);
            return true;
        }

        return false;
    }
};

template <>
struct convert<cgen::config::Template> {
    static auto decode(const Node& node, cgen::config::Template& tpl) -> bool {
        if (node.IsScalar()) {
            const std::string name = as<std::string>(node);
            tpl.names.push_back(name);
        } else {
            tpl.names = as<std::vector<std::string>>(node["names"]);
            tpl.parameters =
                as<std::map<std::string, std::string, std::less<void>>>(node["parameters"]);
        }

        return true;
    }
};

template <>
struct convert<cgen::config::LibraryTarget> {
    static auto decode(const Node& node, cgen::config::LibraryTarget& target) -> bool {
        target.type =
            as<cgen::config::LibraryType>(node["type"], cgen::config::LibraryType::Static);
        target.aliases = as<std::vector<std::string>>(node["aliases"]);
        target.target_settings = as<cgen::config::TargetSettings>(node);
        return true;
    }
};

template <>
struct convert<cgen::config::LibraryType> {
    static auto decode(const Node& node, cgen::config::LibraryType& type) -> bool {
        const std::string type_str = as<std::string>(node);

        if (type_str == "static") {
            type = cgen::config::LibraryType::Static;
            return true;
        }

        if (type_str == "shared") {
            type = cgen::config::LibraryType::Shared;
            return true;
        }

        if (type_str == "interface") {
            type = cgen::config::LibraryType::Interface;
            return true;
        }

        if (type_str == "object") {
            type = cgen::config::LibraryType::Object;
            return true;
        }

        return false;
    }
};

template <>
struct convert<cgen::config::ExecutableTarget> {
    static auto decode(const Node& node, cgen::config::ExecutableTarget& target) -> bool {
        target.target_settings = as<cgen::config::TargetSettings>(node);
        return true;
    }
};

template <>
struct convert<cgen::config::TargetSettings> {
    static auto decode(const Node& node, cgen::config::TargetSettings& settings) -> bool {
        settings.node = node;

        settings.path = as<cgen::config::Expression>(node["path"]);
        settings.options = as<std::map<std::string, cgen::config::Option>>(node["options"]);
        settings.settings = as<std::map<std::string, cgen::config::Expression>>(node["settings"]);

        // with configs
        cgen::node_wrap_configs(node, "properties");
        settings.properties = as<cgen::config::ConfigsExpressionsMap>(node["properties"]);

        // with visibility
        settings.sources =
            as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "sources");
        settings.includes =
            as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "includes");
        settings.pchs = as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "pchs");
        settings.dependencies =
            as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "dependencies");
        settings.definitions =
            as_visibility<cgen::config::VisibilityConfigsDefinitions>(node, "definitions");
        settings.compile_options =
            as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "compile_options");
        settings.link_options =
            as_visibility<cgen::config::VisibilityConfigsExpressions>(node, "link_options");

        return true;
    }

    template <typename T>
    static auto as_visibility(const Node& node, const std::string& key) -> T {
        cgen::node_wrap_visibility(node, key);

        T visibility = as<T>(node[key]);

        // resolve visibility according to the library type
        // if type is missing or target is an executable,
        // fallback to the static library type (ugly, but it works)
        const cgen::config::LibraryType type = as(node["type"], cgen::config::LibraryType::Static);
        switch (type) {
            case cgen::config::LibraryType::Interface:
                // for interface libraries everything is interface
                visibility.interface.move_merge(visibility.default_);
                break;
            default:
                // by default everything is private
                visibility.private_.move_merge(visibility.default_);
        }

        return visibility;
    }
};

template <>
struct convert<cgen::config::VisibilityConfigsExpressions> {
    static auto decode(const Node& node, cgen::config::VisibilityConfigsExpressions& visibility)
        -> bool {
        visibility.default_ = as<cgen::config::ConfigsExpressions>(node["default"]);
        visibility.public_ = as<cgen::config::ConfigsExpressions>(node["public"]);
        visibility.private_ = as<cgen::config::ConfigsExpressions>(node["private"]);
        visibility.interface = as<cgen::config::ConfigsExpressions>(node["interface"]);
        return true;
    }
};

template <>
struct convert<cgen::config::ConfigsExpressions> {
    static auto decode(const Node& node, cgen::config::ConfigsExpressions& configs) {
        configs.is_defined = node.IsDefined();
        configs.global = as<std::vector<cgen::config::Expression>>(node["global"]);
        configs.configurations = as<std::map<std::string, std::vector<cgen::config::Expression>>>(
            node["configurations"]);
        return true;
    }
};

template <>
struct convert<cgen::config::VisibilityConfigsExpressionsMap> {
    static auto decode(const Node& node, cgen::config::VisibilityConfigsExpressionsMap& visibility)
        -> bool {
        visibility.default_ = as<cgen::config::ConfigsExpressionsMap>(node["default"]);
        visibility.public_ = as<cgen::config::ConfigsExpressionsMap>(node["public"]);
        visibility.private_ = as<cgen::config::ConfigsExpressionsMap>(node["private"]);
        visibility.interface = as<cgen::config::ConfigsExpressionsMap>(node["interface"]);
        return true;
    }
};

template <>
struct convert<cgen::config::ConfigsExpressionsMap> {
    static auto decode(const Node& node, cgen::config::ConfigsExpressionsMap& configs) -> bool {
        configs.is_defined = node.IsDefined();
        configs.global = as<std::map<std::string, cgen::config::Expression>>(node["global"]);
        configs.configurations =
            as<std::map<std::string, std::map<std::string, cgen::config::Expression>>>(
                node["configurations"]);
        return true;
    }
};

template <>
struct convert<cgen::config::VisibilityConfigsDefinitions> {
    static auto decode(const Node& node, cgen::config::VisibilityConfigsDefinitions& visibility)
        -> bool {
        visibility.default_ = as<cgen::config::ConfigsDefinitions>(node["default"]);
        visibility.public_ = as<cgen::config::ConfigsDefinitions>(node["public"]);
        visibility.private_ = as<cgen::config::ConfigsDefinitions>(node["private"]);
        visibility.interface = as<cgen::config::ConfigsDefinitions>(node["interface"]);
        return true;
    }
};

template <>
struct convert<cgen::config::ConfigsDefinitions> {
    static auto decode(const Node& node, cgen::config::ConfigsDefinitions& configs) -> bool {
        configs.is_defined = node.IsDefined();
        configs.global = as<std::vector<cgen::config::Definition>>(node["global"]);
        configs.configurations = as<std::map<std::string, std::vector<cgen::config::Definition>>>(
            node["configurations"]);
        return true;
    }
};

template <>
struct convert<cgen::config::Definition> {
    static auto decode(const Node& node, cgen::config::Definition& def) -> bool {
        if (node.IsScalar()) {
            def.value = as<cgen::config::Expression>(node);
            return true;
        }

        if (node.IsMap()) {
            def.map = as<std::map<std::string, cgen::config::Expression>>(node);
            return true;
        }

        return false;
    }
};

template <>
struct convert<cgen::config::Expression> {
    static auto decode(const Node& node, cgen::config::Expression& expr) -> bool {
        expr.is_defined = node.IsDefined();
        expr.is_quoted = node.Tag() == "!";
        expr.value = as<std::string>(node);
        return true;
    }
};

}  // namespace YAML
