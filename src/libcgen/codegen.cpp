#include "codegen.hpp"
#include "version.hpp"

#include <poost/assert.hpp>
#include <poost/log.hpp>

#include <algorithm>

namespace cgen {

auto quote(const std::string &str) -> std::string;
auto expression(const config::Expression &expr, bool padded = true) -> std::string;
auto concatenate_paths(const config::Expression &lhs, const config::Expression &rhs)
    -> config::Expression;

auto config_has_packages(const Config &config, config::PackageType type) -> bool;
auto config_target_options(const Config &config)
    -> std::map<std::string, std::map<std::string, config::Option>>;

/// Public

CMakeGenerator::CMakeGenerator(std::ostream &out)
    : m_out{out}, m_indent{0}, m_last_is_blank{false} {}

void CMakeGenerator::write(const Config &config) {
    POOST_TRACE("begin codegen");

    comment("Generated using cgen " + version_string() + " — https://gitlab.com/madyanov/cgen");
    comment("DO NOT EDIT");
    blank();

    version(version::cmake);
    project(config.project);

    if (!config.options.empty()) {
        POOST_TRACE("write options");
        section("Options");
        for (const auto &[opt_name, opt] : config.options) {
            option(opt_name, opt);
        }
    }

    const std::map<std::string, std::map<std::string, config::Option>> target_opts =
        config_target_options(config);
    if (!target_opts.empty()) {
        POOST_TRACE("write target options");
        section("Target options");
        for (const auto &[target_name, opts] : target_opts) {
            blank();
            comment("options for target " + target_name);
            for (const auto &[opt_name, opt] : opts) {
                option(opt_name, opt);
            }
        }
    }

    if (!config.settings.empty()) {
        POOST_TRACE("write settings");
        section("Settings");
        for (const auto &[var_name, expr] : config.settings) {
            set(var_name, expr);
        }
    }

    if (config_has_packages(config, config::PackageType::System)) {
        POOST_TRACE("write system packages");
        section("System packages");
        for (const config::Package &pkg : config.packages) {
            if (pkg.type != config::PackageType::System) {
                continue;
            }

            // clang-format off
            if_begin(pkg.if_);
                find_package(pkg.name, pkg.system);
            if_end(pkg.if_);
            // clang-format on
        }
    }

    if (config_has_packages(config, config::PackageType::External)) {
        POOST_TRACE("write external packages");
        section("External packages");
        int pkg_idx = 0;
        for (const config::Package &pkg : config.packages) {
            if (pkg.type != config::PackageType::External) {
                continue;
            }

            // clang-format off
            blank();
            comment("package " + pkg.name);
            const std::string func_name = "cgen_package_" + std::to_string(pkg_idx++);
            function_begin(func_name);
                for (const auto &[name, expr] : pkg.external.options) {
                    set(name, expr, true);
                }

                const std::filesystem::path source_root{"${PROJECT_SOURCE_DIR}"};
                const std::filesystem::path cmake_lists{"CMakeLists.txt"};
                if_begin("EXISTS " + (source_root / pkg.name / cmake_lists).string());
                    add_subdirectory(pkg.name);
                if_else();
                    warning("Package " + pkg.name + " doesn't have CMakeLists.txt");
                if_end();
            function_end();

            if_begin(pkg.if_);
                function_call(func_name);
            if_end(pkg.if_);
            // clang-format on
        }
    }

    if (!config.targets.empty()) {
        POOST_TRACE("write targets");
        section("Targets");
        int target_idx = 0;
        for (const config::Target &target : config.targets) {
            // clang-format off
            blank();
            comment("target " + target.name);
            const std::string func_name = "cgen_target_" + std::to_string(target_idx++);
            function_begin(func_name);
                switch (target.type) {
                case config::TargetType::Library:
                    for (const auto &[var_name, expr] : target.library.target_settings.settings) {
                        set(var_name, expr);
                    }

                    add_library(target.name, target.library.type);

                    for (const std::string &alias : target.library.aliases) {
                        add_library_alias(target.name, alias);
                    }

                    target_settings(target.name, target.library.target_settings);
                    break;
                case config::TargetType::Executable:
                    for (const auto &[var_name, expr] : target.executable.target_settings.settings) {
                        set(var_name, expr);
                    }

                    add_executable(target.name);
                    target_settings(target.name, target.executable.target_settings);
                    break;
                default:
                    POOST_ASSERT_FAIL("invalid target type: %d", target.type);
                }
            function_end();

            if_begin(target.if_);
                function_call(func_name);
            if_end(target.if_);
            // clang-format on
        }
    }

    POOST_TRACE("end codegen");
}

/// Private

void CMakeGenerator::indent() { ++m_indent; }

void CMakeGenerator::unindent() {
    --m_indent;
    POOST_ASSERT(m_indent >= 0, "negative indentation: %d", m_indent);
}

void CMakeGenerator::line(const std::string &str) {
    m_out << std::string(m_indent * 4, ' ') << str << "\n";
    m_last_is_blank = false;
}

void CMakeGenerator::blank() {
    if (m_last_is_blank) {
        return;
    }

    m_out << "\n";
    m_last_is_blank = true;
}

void CMakeGenerator::comment(const std::string &str) {
    if (!str.empty()) {
        line("# " + str);
    } else {
        line("#");
    }
}

void CMakeGenerator::section(const std::string &str) {
    blank();
    comment();
    comment(str);
    comment();
    blank();
}

void CMakeGenerator::warning(const std::string &msg) {
    line("message(WARNING " + quote(msg) + ")");
}

void CMakeGenerator::if_begin(const std::string &cond) {
    if (cond.empty()) {
        return;
    }

    line("if(" + cond + ")");
    indent();
}

void CMakeGenerator::if_end(const std::string &cond) {
    if (cond.empty()) {
        return;
    }

    unindent();
    line("endif()");
}

void CMakeGenerator::if_else() {
    unindent();
    line("else()");
    indent();
}

void CMakeGenerator::if_end() {
    unindent();
    line("endif()");
}

void CMakeGenerator::function_begin(const std::string &func_name) {
    line("function(" + func_name + ")");
    indent();
}

void CMakeGenerator::function_end() {
    unindent();
    line("endfunction()");
}

void CMakeGenerator::function_call(const std::string &func_name) { line(func_name + "()"); }

void CMakeGenerator::version(const std::string &ver) {
    line("cmake_minimum_required(VERSION " + ver + ")");
}

void CMakeGenerator::project(const config::Project &project) {
    std::string args;

    if (!project.version.empty()) {
        args += " VERSION " + project.version;
    }

    line("project(" + project.name + args + ")");
}

void CMakeGenerator::option(const std::string &opt_name, const config::Option &opt) {
    line("option(" + opt_name + " " + quote(opt.description) + expression(opt.default_) + ")");
}

void CMakeGenerator::set(const std::string &var_name, const config::Expression &expr, bool force) {
    std::string args;

    if (force) {
        args += " CACHE INTERNAL " + quote("") + " FORCE";
    }

    line("set(" + var_name + expression(expr) + args + ")");
}

void CMakeGenerator::find_package(const std::string &pkg_name, const config::SystemPackage &pkg) {
    std::string args;

    if (!pkg.version.empty()) {
        args += " " + pkg.version;
    }

    if (pkg.is_required) {
        args += " REQUIRED";
    }

    line("find_package(" + pkg_name + args + ")");
}

void CMakeGenerator::add_subdirectory(const std::filesystem::path &path) {
    line("add_subdirectory(" + path.string() + ")");
}

void CMakeGenerator::add_library(const std::string &target_name, config::LibraryType type) {
    std::string type_str;
    switch (type) {
    case config::LibraryType::Static:
        type_str = "STATIC";
        break;
    case config::LibraryType::Shared:
        type_str = "SHARED";
        break;
    case config::LibraryType::Interface:
        type_str = "INTERFACE";
        break;
    case config::LibraryType::Object:
        type_str = "OBJECT";
        break;
    default:
        POOST_ASSERT_FAIL("invalid library type: %d", type);
    }

    line("add_library(" + target_name + " " + type_str + ")");
}

void CMakeGenerator::add_library_alias(const std::string &target_name,
                                       const std::string &target_alias) {

    line("add_library(" + target_alias + " ALIAS " + target_name + ")");
}

void CMakeGenerator::add_executable(const std::string &target_name) {
    line("add_executable(" + target_name + ")");
}

void CMakeGenerator::target_settings(const std::string &target_name,
                                     const config::TargetSettings &target_settings) {

    // clang-format off
    if (!target_settings.sources.empty()) {
        target_sources_begin(target_name);
            visibility(target_settings.sources, target_settings.path);
        target_settings_end();
    }

    if (!target_settings.includes.empty()) {
        target_includes_begin(target_name);
            visibility(target_settings.includes, target_settings.path);
        target_settings_end();
    }

    if (!target_settings.pchs.empty()) {
        target_pchs_begin(target_name);
            visibility(target_settings.pchs, target_settings.path);
        target_settings_end();
    }

    if (!target_settings.dependencies.empty()) {
        target_link_libraries_begin(target_name);
            visibility(target_settings.dependencies);
        target_settings_end();
    }

    if (!target_settings.definitions.empty()) {
        target_compile_definitions_begin(target_name);
            visibility(target_settings.definitions);
        target_settings_end();
    }

    if (!target_settings.properties.empty()) {
        target_properties_begin(target_name);
            configs(target_settings.properties);
        target_settings_end();
    }

    if (!target_settings.compile_options.empty()) {
        target_compile_options_begin(target_name);
            visibility(target_settings.compile_options);
        target_settings_end();
    }

    if (!target_settings.link_options.empty()) {
        target_link_options_begin(target_name);
            visibility(target_settings.link_options);
        target_settings_end();
    }
    // clang-format on
}

void CMakeGenerator::target_sources_begin(const std::string &target_name) {
    line("target_sources(" + target_name);
    indent();
}

void CMakeGenerator::target_includes_begin(const std::string &target_name) {
    line("target_include_directories(" + target_name);
    indent();
}

void CMakeGenerator::target_pchs_begin(const std::string &target_name) {
    line("target_precompiled_headers(" + target_name);
    indent();
}

void CMakeGenerator::target_link_libraries_begin(const std::string &target_name) {
    line("target_link_libraries(" + target_name);
    indent();
}

void CMakeGenerator::target_compile_definitions_begin(const std::string &target_name) {
    line("target_compile_definitions(" + target_name);
    indent();
}

void CMakeGenerator::target_properties_begin(const std::string &target_name) {
    line("set_target_properties(" + target_name + " PROPERTIES");
    indent();
}

void CMakeGenerator::target_compile_options_begin(const std::string &target_name) {
    line("target_compile_options(" + target_name);
    indent();
}

void CMakeGenerator::target_link_options_begin(const std::string &target_name) {
    line("target_link_options(" + target_name);
    indent();
}

void CMakeGenerator::target_settings_end() {
    unindent();
    line(")");
}

template <typename T>
void CMakeGenerator::visibility(const config::Visibility<config::Configs<T>> &visibility,
                                const config::Expression &prefix) {

    // clang-format off
    if (!visibility.public_.empty()) {
        line("PUBLIC");
        indent();
            configs(visibility.public_, prefix);
        unindent();
    }

    if (!visibility.interface.empty()) {
        line("INTERFACE");
        indent();
            configs(visibility.interface, prefix);
        unindent();
    }

    if (!visibility.private_.empty()) {
        line("PRIVATE");
        indent();
            configs(visibility.private_, prefix);
        unindent();
    }
    // clang-format on
}

void CMakeGenerator::configs(const config::ConfigsExpressions &configs,
                             const config::Expression &prefix) {

    for (const config::Expression &expr : configs.global) {
        line(expression(concatenate_paths(prefix, expr), false));
    }

    for (const auto &[config, exprs] : configs.configurations) {
        if (exprs.empty()) {
            continue;
        }

        config_begin(config);

        for (const config::Expression &expr : exprs) {
            line(expression(concatenate_paths(prefix, expr), false));
        }

        config_end();
    }
}

void CMakeGenerator::configs(const config::ConfigsDefinitions &configs,
                             const config::Expression &) {

    for (const config::Definition &def : configs.global) {
        definition(def);
    }

    for (const auto &[config, defs] : configs.configurations) {
        if (defs.empty()) {
            continue;
        }

        config_begin(config);

        for (const config::Definition &def : defs) {
            definition(def);
        }

        config_end();
    }
}

void CMakeGenerator::configs(const config::ConfigsExpressionsMap &configs,
                             const config::Expression &) {

    for (const auto &[key, expr] : configs.global) {
        line(key + expression(expr));
    }

    for (const auto &[config, map] : configs.configurations) {
        if (map.empty()) {
            continue;
        }

        config_begin(config);

        for (const auto &[key, expr] : map) {
            line(key + expression(expr));
        }

        config_end();
    }
}

void CMakeGenerator::config_begin(const std::string &config_name) {
    line("$<$<CONFIG:" + config_name + ">:");
    indent();
}

void CMakeGenerator::config_end() {
    unindent();
    line(">");
}

void CMakeGenerator::definition(const config::Definition &def) {
    if (def.map.empty()) {
        line(expression(def.value, false));
    } else {
        for (const auto &[key, expr] : def.map) {
            line(key + "=" + expression(expr, false));
        }
    }
}

/// Utility

auto quote(const std::string &str) -> std::string { return '"' + str + '"'; }

auto expression(const config::Expression &expr, bool padded) -> std::string {
    std::string result;

    if (!expr.is_defined) {
        return result;
    }

    if (padded) {
        result += " ";
    }

    if (expr.is_quoted) {
        result += quote(expr.value);
    } else {
        result += expr.value;
    }

    return result;
}

auto concatenate_paths(const config::Expression &lhs, const config::Expression &rhs)
    -> config::Expression {

    const std::filesystem::path lhs_path{lhs.value};
    const std::filesystem::path rhs_path{rhs.value};

    return config::Expression{
        lhs.is_defined || rhs.is_defined,
        lhs.is_quoted || rhs.is_quoted,
        lhs_path / rhs_path,
    };
}

auto config_has_packages(const Config &config, config::PackageType type) -> bool {
    return std::any_of(config.packages.cbegin(), config.packages.cend(),
                       [type](const config::Package &pkg) -> bool { return pkg.type == type; });
}

auto config_target_options(const Config &config)
    -> std::map<std::string, std::map<std::string, config::Option>> {

    std::map<std::string, std::map<std::string, config::Option>> target_opts;

    for (const config::Target &target : config.targets) {
        switch (target.type) {
        case config::TargetType::Library:
            if (!target.library.target_settings.options.empty()) {
                target_opts[target.name] = target.library.target_settings.options;
            }

            break;
        case config::TargetType::Executable:
            if (!target.executable.target_settings.options.empty()) {
                target_opts[target.name] = target.executable.target_settings.options;
            }

            break;
        default:
            POOST_ASSERT_FAIL("invalid target type: %d", target.type);
        }
    }

    return target_opts;
}

} // namespace cgen
