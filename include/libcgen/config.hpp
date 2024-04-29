#ifndef CGEN_CONFIG_HPP
#define CGEN_CONFIG_HPP

#include "error.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <functional>
#include <istream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace cgen {

namespace config {

struct Expression {
    bool is_defined;
    bool is_quoted;
    std::string value;

    Expression() : is_defined{false}, is_quoted{false} {}
    Expression(bool is_defined, bool is_quoted, const std::string &val)
        : is_defined{is_defined}, is_quoted{is_quoted}, value{val} {}
};

struct Project {
    std::string name;
    std::string version;
};

struct Include {
    std::vector<std::string> paths;
    std::map<std::string, std::string, std::less<void>> parameters;
};

struct Option {
    std::string description;
    Expression default_;
};

enum class PackageType {
    External,
    System,
};

enum class FetchStrategy {
    Submodule,
    Clone,
};

struct ExternalPackage {
    std::string url;
    std::string version;
    FetchStrategy strategy;
    std::map<std::string, Expression> options;
};

struct SystemPackage {
    std::string version;
    bool is_required;
};

struct Package {
    PackageType type;
    std::string name;
    std::string if_;
    ExternalPackage external;
    SystemPackage system;
};

enum class TargetType {
    Library,
    Executable,
};

struct Template {
    std::vector<std::string> names;
    std::map<std::string, std::string, std::less<void>> parameters;
};

enum class LibraryType {
    Static,
    Shared,
    Interface,
    Object,
};

template <typename T> struct Visibility {
    T default_;
    T public_;
    T private_;
    T interface;

    auto empty() const -> bool {
        return public_.empty() && private_.empty() && interface.empty();
    }
};

template <typename T> struct Configs {
    bool is_defined;

    T global;
    std::map<std::string, T> configurations;

    Configs() : is_defined{false} {}

    auto empty() const -> bool {
        return !is_defined ||
               (global.empty() &&
                std::all_of(
                    configurations.cbegin(), configurations.cend(),
                    [](const auto &it) -> bool { return it.second.empty(); }));
    }

    void move_merge(const Configs<T> &configs) {
        is_defined = is_defined || configs.is_defined;
        global.insert(global.end(),
                      std::make_move_iterator(configs.global.cbegin()),
                      std::make_move_iterator(configs.global.cend()));
        configurations.insert(
            std::make_move_iterator(configs.configurations.cbegin()),
            std::make_move_iterator(configs.configurations.cend()));
    }
};

struct Definition {
    Expression value;
    std::map<std::string, Expression> map;
};

using ConfigsExpressions = Configs<std::vector<Expression>>;
using ConfigsExpressionsMap = Configs<std::map<std::string, Expression>>;
using ConfigsDefinitions = Configs<std::vector<Definition>>;
using VisibilityConfigsExpressions = Visibility<ConfigsExpressions>;
using VisibilityConfigsExpressionsMap = Visibility<ConfigsExpressionsMap>;
using VisibilityConfigsDefinitions = Visibility<ConfigsDefinitions>;

struct TargetSettings {
    YAML::Node node;
    Expression path;
    std::map<std::string, Option> options;
    std::map<std::string, Expression> settings;
    VisibilityConfigsExpressions sources;
    VisibilityConfigsExpressions includes;
    VisibilityConfigsExpressions pchs;
    VisibilityConfigsExpressions dependencies;
    VisibilityConfigsDefinitions definitions;
    ConfigsExpressionsMap properties;
    VisibilityConfigsExpressions compile_options;
    VisibilityConfigsExpressions link_options;
};

struct LibraryTarget {
    LibraryType type;
    std::vector<std::string> aliases;
    TargetSettings target_settings;
};

struct ExecutableTarget {
    TargetSettings target_settings;
};

struct Target {
    YAML::Node node;
    TargetType type;
    std::string name;
    std::string if_;
    std::vector<Template> templates;
    LibraryTarget library;
    ExecutableTarget executable;
};

} // namespace config

struct Config {
    std::string version;
    config::Project project;
    std::vector<config::Include> includes;
    std::map<std::string, config::TargetSettings> templates;
    std::map<std::string, config::Option> options;
    std::map<std::string, config::Expression> settings;
    std::vector<config::Package> packages;
    std::vector<config::Target> targets;
};

auto config_read(std::istream &in, int ver,
                 std::vector<Error> &errors) -> Config;

} // namespace cgen

#endif // ifndef CGEN_CONFIG_HPP
