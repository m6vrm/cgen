#ifndef CGEN_CODEGEN_HPP
#define CGEN_CODEGEN_HPP

#include "config.hpp"

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace cgen {

class CMakeGenerator {
  public:
    explicit CMakeGenerator(std::ostream &out);

    void write(const Config &config);

  private:
    void indent();
    void unindent();

    void line(const std::string &str);
    void blank();

    void comment(const std::string &str = "");
    void section(const std::string &str);

    void notice(const std::string &msg);

    void if_begin(const std::string &cond);
    void if_end(const std::string &cond);
    void if_else();
    void if_end();
    void function_begin(const std::string &func_name);
    void function_end();
    void function_call(const std::string &func_name);

    void version(const std::string &ver);
    void project(const config::Project &project);
    void option(const std::string &opt_name, const config::Option &opt);
    void set(const std::string &var_name, const config::Expression &expr,
             bool force = false);
    void find_package(const std::string &pkg_name,
                      const config::SystemPackage &pkg);

    void add_subdirectory(const std::filesystem::path &path);
    void add_library(const std::string &target_name, config::LibraryType type);
    void add_library_alias(const std::string &target_name,
                           const std::string &target_alias);
    void add_executable(const std::string &target_name);

    void target_settings(const std::string &target_name,
                         const config::TargetSettings &target_settings);
    void target_sources_begin(const std::string &target_name);
    void target_includes_begin(const std::string &target_name);
    void target_pchs_begin(const std::string &target_name);
    void target_link_libraries_begin(const std::string &target_name);
    void target_compile_definitions_begin(const std::string &target_name);
    void target_properties_begin(const std::string &target_name);
    void target_compile_options_begin(const std::string &target_name);
    void target_link_options_begin(const std::string &target_name);
    void target_settings_end();

    template <typename T>
    void visibility(const config::Visibility<config::Configs<T>> &visibility,
                    const config::Expression &prefix = {});

    void configs(const config::ConfigsExpressions &configs,
                 const config::Expression &prefix);
    void configs(const config::ConfigsDefinitions &configs,
                 const config::Expression &);
    void configs(const config::ConfigsExpressionsMap &configs,
                 const config::Expression & = {});

    void config_begin(const std::string &config_name);
    void config_end();

    void definition(const config::Definition &def);

  private:
    std::ostream &m_out;
    int m_indent;
    bool m_last_is_blank;
};

} // namespace cgen

#endif // ifndef CGEN_CODEGEN_HPP
