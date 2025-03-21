#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libcgen/codegen.hpp>
#include <libcgen/config.hpp>
#include <libcgen/error.hpp>
#include <libcgen/packages.hpp>
#include <libcgen/version.hpp>
#include <poost/args.hpp>
#include <poost/assert.hpp>
#include <poost/log.hpp>
#include <vector>

inline constexpr char config_file[] = "cgen.yml";
inline constexpr char resolved_file[] = "cgen.resolved";
inline constexpr char cmake_file[] = "CMakeLists.txt";

static auto use_colors() -> bool;

const poost::LogSettings log_common{
    .stream = &std::cerr,
    .log_level = poost::LogLevel::INFO,
    .prefix = nullptr,
    .use_colors = use_colors(),
    .print_location = false,
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

static auto command_generate() -> bool;
static auto command_update(const std::vector<std::filesystem::path>& paths) -> bool;

static auto config_read(cgen::Config& config,
                        std::vector<cgen::Package>& pkgs,
                        std::vector<cgen::Package>& resolved_pkgs) -> bool;

static auto resolved_write(const std::vector<cgen::Package>& old_resolved_pkgs,
                           const std::vector<cgen::Package>& new_resolved_pkgs) -> bool;

static auto cmake_write(const cgen::Config& config) -> bool;

static void errors_print(const std::vector<cgen::Error>& errors);
static auto arguments_parse(int argc, const char* argv[], Options& opts) -> ArgumentsParseResult;

static void print_usage(const char* argv0);

auto main(int argc, const char* argv[]) -> int {
    POOST_INFO_EX(log_common, "cgen {}", cgen::version_string());

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

    // global log settings
    poost::log::global = poost::LogSettings{
        .stream = &std::cerr,
        .log_level = opts.verbose ? poost::LogLevel::ALL : poost::LogLevel::FATAL,
        .prefix = nullptr,
        .use_colors = use_colors(),
        .print_location = opts.verbose,
    };

    // run command
    switch (opts.command) {
        case Command::Generate:
            return command_generate() ? EXIT_SUCCESS : EXIT_FAILURE;
        case Command::Update:
            return command_update(opts.packages) ? EXIT_SUCCESS : EXIT_FAILURE;
        default:
            POOST_ERROR_EX(log_common, "please specify command");
            print_usage(argv[0]);
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

static auto command_update(const std::vector<std::filesystem::path>& paths) -> bool {
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

static auto config_read(cgen::Config& config,
                        std::vector<cgen::Package>& pkgs,
                        std::vector<cgen::Package>& resolved_pkgs) -> bool {
    // open config
    std::ifstream config_in{config_file};
    if (config_in.fail()) {
        POOST_ERROR_EX(log_common, "can't access config file: {}", config_file);
        return false;
    }

    // read config
    POOST_INFO_EX(log_common, "read config file: {}", config_file);
    std::vector<cgen::Error> errors;
    config = cgen::config_read(config_in, cgen::version::major, errors);
    if (!errors.empty()) {
        errors_print(errors);
        return false;
    }

    // extract external packages
    for (const cgen::config::Package& pkg : config.packages) {
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
                POOST_ASSERT_FAIL("invalid package fetch strategy: {}", pkg.external.strategy);
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
        POOST_INFO_EX(log_common, "read resolved file: {}", resolved_file);
        resolved_pkgs = cgen::resolved_read(resolved_in);
        resolved_pkgs = cgen::packages_cleanup(pkgs, resolved_pkgs);
    }

    return true;
}

static auto resolved_write(const std::vector<cgen::Package>& old_resolved_pkgs,
                           const std::vector<cgen::Package>& new_resolved_pkgs) -> bool {
    if (old_resolved_pkgs.empty() && new_resolved_pkgs.empty()) {
        return true;
    }

    POOST_INFO_EX(log_common, "write resolved file: {}", resolved_file);
    std::ofstream out{resolved_file, std::ostream::trunc};
    const std::vector<cgen::Package> resolved_pkgs =
        cgen::packages_merge(old_resolved_pkgs, new_resolved_pkgs);
    cgen::resolved_write(out, resolved_pkgs);
    return !!out;
}

static auto cmake_write(const cgen::Config& config) -> bool {
    POOST_INFO_EX(log_common, "generate and write cmake file: {}", cmake_file);
    std::ofstream out{cmake_file, std::ostream::trunc};
    cgen::CMakeGenerator cmake{out};
    cmake.write(config);
    return !!out;
}

static void errors_print(const std::vector<cgen::Error>& errors) {
    for (const cgen::Error& err : errors) {
        POOST_ERROR_EX(log_common, "{}", err.description());
    }
}

static auto arguments_parse(int argc, const char* argv[], Options& opts) -> ArgumentsParseResult {
    ArgumentsParseResult result = ArgumentsParseResult::SuccessExit;

    poost::Args args{argc, argv};
    for (char opt = args.option(); opt != poost::args::end; opt = args.option()) {
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
                    POOST_ERROR_EX(log_common, "invalid argument: {}", args.peek());
                } else {
                    POOST_ERROR_EX(log_common, "unknown option: {}", opt);
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

static void print_usage(const char* argv0) {
    printf(
        "usage: %s [-g] [-u package ...] [-v] [-h]"
        "\n  -g  generate CMakeLists.txt and fetch external packages"
        "\n  -u  update external packages"
        "\n  -v  verbose output"
        "\n  -h  show this help"
        "\n",
        argv0);
}

static auto use_colors() -> bool {
    // respect NO_COLOR environment variable
    // see: https://no-color.org
    const char* no_color = std::getenv("NO_COLOR");
    return no_color == nullptr || no_color[0] == '\0';
}
