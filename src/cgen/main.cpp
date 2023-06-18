#include <libcgen/codegen.hpp>
#include <libcgen/config.hpp>
#include <libcgen/error.hpp>
#include <libcgen/packages.hpp>
#include <libcgen/version.hpp>

#include <poost/args.hpp>
#include <poost/assert.hpp>
#include <poost/log.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

inline constexpr char config_file[] = ".cgen.yml";
inline constexpr char resolved_file[] = ".cgen.resolved";
inline constexpr char cmake_file[] = "CMakeLists.txt";

auto use_color() -> bool;

const poost::LogSettings log_common{
    .stream = stderr,
    .log_level = poost::LogLevel::Info,
    .print_file_line = false,
    .use_color = use_color(),
};

enum class ArgumentsParseResult {
    SuccessContinue,
    SuccessExit,
    FailureExit,
};

enum class Command {
    Unspecified,
    Generate,
    Update,
};

struct Options {
    Command command;
    std::vector<std::filesystem::path> packages;
    bool verbose;
};

auto command_generate() -> bool;
auto command_update(const std::vector<std::filesystem::path> &paths) -> bool;

auto config_read(cgen::Config &config, std::vector<cgen::Package> &pkgs,
                 std::vector<cgen::Package> &resolved_pkgs) -> bool;

auto resolved_write(const std::vector<cgen::Package> &old_resolved_pkgs,
                    const std::vector<cgen::Package> &new_resolved_pkgs) -> bool;

auto cmake_write(const cgen::Config &config) -> bool;

void errors_print(const std::vector<cgen::Error> &errors);
auto arguments_parse(int argc, char *argv[], Options &opts) -> ArgumentsParseResult;

void print_usage(const char *argv0);

auto main(int argc, char *argv[]) -> int {
    POOST_INFO_EX(log_common, "cgen %s", cgen::version_string().c_str());

    // parse arguments
    Options opts{};

    switch (arguments_parse(argc, argv, opts)) {
    case ArgumentsParseResult::SuccessExit:
        return EXIT_SUCCESS;
    case ArgumentsParseResult::FailureExit:
        return EXIT_FAILURE;
    case ArgumentsParseResult::SuccessContinue:
        // do nothing
        break;
    }

    // main log settings
    if (opts.verbose) {
        poost::log::main = poost::LogSettings{
            .stream = stderr,
            .log_level = poost::LogLevel::All,
            .print_file_line = true,
            .use_color = use_color(),
        };
    } else {
        poost::log::main = poost::LogSettings{
            .stream = stderr,
            .log_level = poost::LogLevel::Fatal,
            .print_file_line = false,
            .use_color = use_color(),
        };
    }

    // run command
    switch (opts.command) {
    case Command::Generate:
        return command_generate() ? EXIT_SUCCESS : EXIT_FAILURE;
    case Command::Update:
        return command_update(opts.packages) ? EXIT_SUCCESS : EXIT_FAILURE;
    default:
        POOST_ERROR_EX(log_common, "please specify command");
        print_usage(argv[0]);
        break;
    }

    return EXIT_FAILURE;
}

auto command_generate() -> bool {
    // read config
    cgen::Config config{};
    std::vector<cgen::Package> pkgs;
    std::vector<cgen::Package> resolved_pkgs;
    if (!config_read(config, pkgs, resolved_pkgs)) {
        return false;
    }

    // resolve packages
    std::vector<cgen::Error> errors;
    std::vector<cgen::Package> new_resolved_pkgs;
    if (!pkgs.empty()) {
        POOST_INFO_EX(log_common, "resolve packages");
        new_resolved_pkgs = cgen::packages_resolve(pkgs, resolved_pkgs, errors);
    }

    // write resolved
    if (!resolved_write(resolved_pkgs, new_resolved_pkgs)) {
        return false;
    }

    // write cmake
    if (!cmake_write(config)) {
        return false;
    }

    errors_print(errors);
    return errors.empty();
}

auto command_update(const std::vector<std::filesystem::path> &paths) -> bool {
    // read config
    cgen::Config config{};
    std::vector<cgen::Package> pkgs;
    std::vector<cgen::Package> resolved_pkgs;
    if (!config_read(config, pkgs, resolved_pkgs)) {
        return false;
    }

    // update packages
    std::vector<cgen::Error> errors;
    std::vector<cgen::Package> new_resolved_pkgs;
    if (!pkgs.empty()) {
        POOST_INFO_EX(log_common, "update packages");
        new_resolved_pkgs = cgen::packages_update(pkgs, paths, errors);
    } else {
        POOST_INFO_EX(log_common, "nothing to update");
    }

    // write resolved
    if (!resolved_write(resolved_pkgs, new_resolved_pkgs)) {
        return false;
    }

    errors_print(errors);
    return errors.empty();
}

auto config_read(cgen::Config &config, std::vector<cgen::Package> &pkgs,
                 std::vector<cgen::Package> &resolved_pkgs) -> bool {

    // open config
    std::ifstream config_in{config_file};
    if (config_in.fail()) {
        POOST_ERROR_EX(log_common, "can't access config file: %s", config_file);
        return false;
    }

    // read config
    POOST_INFO_EX(log_common, "read config file: %s", config_file);
    std::vector<cgen::Error> errors;
    config = cgen::config_read(config_in, cgen::version::major, errors);
    if (!errors.empty()) {
        errors_print(errors);
        return false;
    }

    // extract external packages
    for (const cgen::config::Package &pkg : config.packages) {
        if (pkg.type != cgen::config::PackageType::External) {
            continue;
        }

        cgen::packages::FetchStrategy strategy;
        switch (pkg.external.strategy) {
        case cgen::config::FetchStrategy::Submodule:
            strategy = cgen::packages::FetchStrategy::Submodule;
            break;
        case cgen::config::FetchStrategy::Clone:
            strategy = cgen::packages::FetchStrategy::Clone;
            break;
        default:
            POOST_ASSERT_FAIL("invalid package fetch strategy: %c", pkg.external.strategy);
        }

        pkgs.push_back(cgen::Package{
            .strategy = strategy,
            .path = pkg.name,
            .url = pkg.external.url,
            .version = pkg.external.version,

            // equals to version before resolving
            .original_version = pkg.external.version.empty() ? "HEAD" : pkg.external.version,
        });
    }

    // read resolved
    std::ifstream resolved_in{resolved_file};
    if (!resolved_in.fail()) {
        POOST_INFO_EX(log_common, "read resolved file: %s", resolved_file);
        resolved_pkgs = cgen::resolved_read(resolved_in);
        resolved_pkgs = cgen::packages_cleanup(pkgs, resolved_pkgs);
    }

    return true;
}

auto resolved_write(const std::vector<cgen::Package> &old_resolved_pkgs,
                    const std::vector<cgen::Package> &new_resolved_pkgs) -> bool {

    POOST_INFO_EX(log_common, "write resolved file: %s", resolved_file);
    std::ofstream out{resolved_file, std::ostream::trunc};
    const std::vector<cgen::Package> resolved_pkgs =
        cgen::packages_merge(old_resolved_pkgs, new_resolved_pkgs);
    cgen::resolved_write(out, resolved_pkgs);
    return true;
}

auto cmake_write(const cgen::Config &config) -> bool {
    POOST_INFO_EX(log_common, "generate and write cmake file: %s", cmake_file);
    std::ofstream out{cmake_file, std::ostream::trunc};
    cgen::CMakeGenerator cmake{out};
    cmake.write(config);
    return true;
}

void errors_print(const std::vector<cgen::Error> &errors) {
    for (const cgen::Error &err : errors) {
        POOST_ERROR_EX(log_common, err.description().c_str());
    }
}

auto arguments_parse(int argc, char *argv[], Options &opts) -> ArgumentsParseResult {
    ArgumentsParseResult result = ArgumentsParseResult::SuccessExit;

    poost::Args args{argc, argv};
    char opt;
    while ((opt = args.option()) != poost::args::end) {
        switch (opt) {
        case 'g':
            // generate
            opts.command = Command::Generate;
            break;
        case 'u': {
            // update
            opts.command = Command::Update;

            std::string pkg;
            while (args.value(pkg)) {
                opts.packages.push_back(pkg);
            }
        } break;
        case 'v':
            // verbosity
            opts.verbose = true;
            break;
        default:
            // argument error
            if (opt == poost::args::not_an_option) {
                POOST_ERROR_EX(log_common, "invalid argument: %s", args.peek());
            } else {
                POOST_ERROR_EX(log_common, "unknown option: %c", opt);
            }

            result = ArgumentsParseResult::FailureExit;
            [[fallthrough]];
        case 'h':
            // usage
            print_usage(argv[0]);
            return result;
        }
    }

    return ArgumentsParseResult::SuccessContinue;
}

void print_usage(const char *argv0) {
    POOST_INFO_EX(log_common,
                  "usage: %s [-g] [-u package ...] [-v] [-h]"
                  "\n"
                  "\n      -g Generate CMakeLists.txt and fetch external packages"
                  "\n      -u Update external packages"
                  "\n      -v Verbose output"
                  "\n      -h Show this help",
                  argv0);
}

auto use_color() -> bool {
    // respect NO_COLOR environment variable
    // see: https://no-color.org
    const char *no_color = std::getenv("NO_COLOR");
    return no_color == nullptr || no_color[0] == '\0';
}
