#include <algorithm>
#include <cstdlib>
#include <fs.hpp>
#include <git.hpp>
#include <packages.hpp>
#include <poost/assert.hpp>
#include <poost/log.hpp>
#include <version.hpp>
#include <versions.hpp>

// todo: write tests

namespace cgen {

static auto packages_find(const std::vector<Package>& pkgs, const Package& pkg)
    -> std::vector<Package>::const_iterator;
static auto packages_contains(const std::vector<Package>& pkgs, const Package& pkg) -> bool;

static void package_remove(const Package& pkg);
static auto package_tag(const Package& pkg, std::vector<Error>& errors) -> std::string;
static auto package_fetch(const Package& pkg, std::vector<Error>& errors) -> Package;

static void package_backup(const Package& pkg);
static void package_backup_remove(const Package& pkg);
static void package_backup_restore(const Package& pkg);

static auto operator>>(std::istream& in, packages::FetchStrategy& strategy) -> std::istream&;

const std::filesystem::path git_modules_path = ".git/modules";
const std::string backup_suffix = ".bak";

/// Packages

auto packages_cleanup(const std::vector<Package>& pkgs, const std::vector<Package>& resolved_pkgs)
    -> std::vector<Package> {
    std::vector<Package> result;

    // remove resolved packages that aren't exist in the config
    for (const Package& pkg : resolved_pkgs) {
        if (packages_contains(pkgs, pkg)) {
            result.push_back(pkg);
        } else {
            package_remove(pkg);
        }
    }

    return result;
}

auto packages_resolve(const std::vector<Package>& pkgs,
                      const std::vector<Package>& resolved_pkgs,
                      std::vector<Error>& errors) -> std::vector<Package> {
    std::vector<Package> result;

    // resolve packages that aren't already resolved
    for (const Package& pkg : pkgs) {
        const auto resolved_it = packages_find(resolved_pkgs, pkg);

        if (resolved_it == resolved_pkgs.cend() ||
            resolved_it->original_version != pkg.original_version ||
            resolved_it->strategy != pkg.strategy) {
            // package not resolved or has different version or fetch strategy
            // fetch new package
            POOST_DEBUG("fetch new package: {}", pkg.url);
            const Package resolved_pkg = package_fetch(pkg, errors);

            if (errors.empty()) {
                result.push_back(resolved_pkg);
            }
        } else if (path_is_empty(pkg.path)) {
            // package resolved but not exists
            // fetch resolved package
            POOST_DEBUG("fetch resolved package: {}", resolved_it->url);
            package_fetch(*resolved_it, errors);

            if (errors.empty()) {
                result.push_back(*resolved_it);
            }
        } else {
            // package already resolved and exists
            POOST_DEBUG("package already resolved and exists: {}", resolved_it->path);
            result.push_back(*resolved_it);
        }
    }

    return result;
}

auto packages_update(const std::vector<Package>& pkgs,
                     const std::vector<std::filesystem::path>& paths,
                     std::vector<Error>& errors) -> std::vector<Package> {
    std::vector<Package> result;

    // update all packages if paths are empty
    const std::size_t i_max = std::max<std::size_t>(1, paths.size());

    for (std::size_t i = 0; i < i_max; ++i) {
        for (const Package& pkg : pkgs) {
            if (i < paths.size() && !path_is_equal(paths[i], pkg.path)) {
                continue;
            }

            POOST_DEBUG("update package: {}", pkg.url);
            const Package resolved_pkg = package_fetch(pkg, errors);

            if (errors.empty()) {
                result.push_back(resolved_pkg);
            }

            if (i < paths.size()) {
                // go to the next path
                goto next_path;
            }
        }

        if (i < paths.size()) {
            const std::filesystem::path path = paths[i];
            POOST_ERROR("package not found: {}", path);
            errors.push_back(Error{
                .type = ErrorType::PackageNotFound,
                .source = "",
                .subject = path,
            });
        }

    next_path:;
    }

    return result;
}

auto packages_merge(const std::vector<Package>& from, const std::vector<Package>& to)
    -> std::vector<Package> {
    std::vector<Package> result = to;

    for (const Package& pkg : from) {
        if (!packages_contains(result, pkg)) {
            result.push_back(pkg);
        }
    }

    return result;
}

/// Resolved

auto resolved_read(std::istream& in) -> std::vector<Package> {
    std::vector<Package> resolved;

    std::string comment;
    int format = 0;
    in >> comment >> format;

    if (format != version::resolved) {
        POOST_WARN("unsupported resolved format: {}", format);
        return resolved;
    }

    Package pkg{};
    while (in >> pkg.strategy >> pkg.path >> pkg.url >> pkg.version >> pkg.original_version) {
        resolved.push_back(pkg);
    }

    return resolved;
}

void resolved_write(std::ostream& out, const std::vector<Package>& resolved_pkgs) {
    // sort packages to get rid of unnecessary changes in the diff
    std::vector<Package> sorted_pkgs = resolved_pkgs;
    std::sort(
        sorted_pkgs.begin(), sorted_pkgs.end(),
        [](const Package& pkg1, const Package& pkg2) -> bool { return pkg1.path < pkg2.path; });

    out << "format\t" << version::resolved << "\n";

    for (const Package& pkg : sorted_pkgs) {
        out << static_cast<char>(pkg.strategy) << "\t";
        out << pkg.path << "\t";
        out << pkg.url << "\t";
        out << pkg.version << "\t";
        out << pkg.original_version << "\n";
    }
}

/// Private

static auto packages_find(const std::vector<Package>& pkgs, const Package& pkg)
    -> std::vector<Package>::const_iterator {
    const std::filesystem::path path = pkg.path;
    return std::find_if(pkgs.cbegin(), pkgs.cend(),
                        [&path](const Package& pkg) -> bool { return pkg.path == path; });
}

static auto packages_contains(const std::vector<Package>& pkgs, const Package& pkg) -> bool {
    return packages_find(pkgs, pkg) != pkgs.cend();
}

static void package_remove(const Package& pkg) {
    if (!path_exists(pkg.path)) {
        return;
    }

    git_submodule_deinit(pkg.path);
    git_remove(pkg.path);

    path_remove(git_modules_path / pkg.path);
    path_remove(pkg.path);
}

static auto package_tag(const Package& pkg, std::vector<Error>& errors) -> std::string {
    POOST_TRACE("get all remote tags: {}", pkg.url);
    std::vector<std::string> tags;
    const int status = git_remote_tags(pkg.url, tags);
    if (status != EXIT_SUCCESS) {
        POOST_ERROR("can't get remote tags: {}", pkg.url);

        errors.push_back(Error{
            .type = ErrorType::PackageVersionResolutionError,
            .source = pkg.url,
            .subject = pkg.version,
        });

        return pkg.version;
    }

    POOST_TRACE("find tag by version: {}", pkg.version);
    const auto tag = version_tag(pkg.version, tags, false);
    if (!tag.has_value()) {
        POOST_ERROR(
            "can't find tag by version: {}"
            "\n\turl: {}",
            pkg.version, pkg.url);

        errors.push_back(Error{
            .type = ErrorType::PackageVersionResolutionError,
            .source = pkg.url,
            .subject = pkg.version,
        });

        return pkg.version;
    }

    return tag.value();
}

static auto package_fetch(const Package& pkg, std::vector<Error>& errors) -> Package {
    Package resolved_pkg = pkg;

    if (!path_is_sub(pkg.path, std::filesystem::current_path())) {
        POOST_FATAL(
            "fetching packages into the paths outside of the current "
            "working dir is "
            "prohibited: {}",
            pkg.path);
        return resolved_pkg;
    }

    package_backup(pkg);

    int status = 0;

    switch (pkg.strategy) {
        case packages::FetchStrategy::Submodule:
            if (pkg.version.empty()) {
                // package version is empty
                POOST_TRACE("add submodule: {}", pkg.url);
                status = git_submodule_add(pkg.path, pkg.url);
            } else if (version_is_valid(pkg.version)) {
                // package version looks like version tag
                const std::string tag = package_tag(pkg, errors);
                if (!errors.empty()) {
                    package_backup_restore(pkg);
                    return resolved_pkg;
                }

                POOST_TRACE(
                    "add submodule: {}"
                    "\n\ttag: {}",
                    pkg.url, tag);
                status = git_submodule_add(pkg.path, pkg.url);
                status = git_reset_hard(pkg.path, tag) | status;
            } else {
                // package version is a branch name or a commit hash
                POOST_TRACE(
                    "add submodule: {}"
                    "\n\tref: {}",
                    pkg.url, pkg.version);
                status = git_submodule_add(pkg.path, pkg.url);
                status = git_reset_hard(pkg.path, pkg.version) | status;
            }

            // pull nested submodules
            status = git_submodule_init(pkg.path) | status;
            break;
        case packages::FetchStrategy::Clone:
            if (pkg.version.empty()) {
                // package version is empty
                POOST_TRACE("shallow clone: {}", pkg.url);
                status = git_clone_shallow(pkg.path, pkg.url);
            } else if (git_is_commit(pkg.version)) {
                // package version is a commit hash
                POOST_TRACE(
                    "full clone: {}"
                    "\n\tcommit: {}",
                    pkg.url, pkg.version);
                status = git_clone_full(pkg.path, pkg.url);
                status = git_reset_hard(pkg.path, pkg.version) | status;
            } else if (version_is_valid(pkg.version)) {
                // package version looks like version tag
                const std::string tag = package_tag(pkg, errors);
                if (!errors.empty()) {
                    package_backup_restore(pkg);
                    return resolved_pkg;
                }

                POOST_TRACE(
                    "clone branch: {}"
                    "\n\ttag: {}",
                    pkg.url, tag);
                status = git_clone_branch(pkg.path, pkg.url, tag);
            } else {
                // package version is a branch name
                POOST_TRACE(
                    "clone branch: {}"
                    "\n\tbranch: {}",
                    pkg.url, pkg.version);
                status = git_clone_branch(pkg.path, pkg.url, pkg.version);
            }

            break;
        default:
            POOST_ASSERT_FAIL("invalid package fetch strategy: {}", pkg.strategy);
    }

    if (status != EXIT_SUCCESS) {
        POOST_ERROR(
            "can't fetch package: {}"
            "\n\texit status: {}",
            pkg.url, status);

        errors.push_back(Error{
            .type = ErrorType::PackageFetchError,
            .source = pkg.url,
            .subject = std::to_string(status),
        });

        package_backup_restore(pkg);
        return resolved_pkg;
    }

    // get commit hash of current HEAD
    POOST_TRACE("resolve commit hash of the current HEAD: {}", pkg.path);
    status = git_resolve_ref(pkg.path, "HEAD", resolved_pkg.version) | status;
    if (status != EXIT_SUCCESS) {
        POOST_ERROR(
            "can't resolve commit hash of current HEAD: {}"
            "\n\texit status: {}",
            pkg.path, status);

        errors.push_back(Error{
            .type = ErrorType::PackageVersionResolutionError,
            .source = pkg.path,
            .subject = std::to_string(status),
        });

        package_backup_restore(pkg);
        return resolved_pkg;
    }

    // must keep the .git *file* for submodules
    if (path_is_dir(pkg.path / ".git")) {
        path_remove(pkg.path / ".git");
    }

    POOST_DEBUG(
        "resolved package"
        "\n\tstrategy: {}"
        "\n\tpath: {}"
        "\n\turl: {}"
        "\n\tcommit: {}",
        resolved_pkg.strategy, resolved_pkg.path, resolved_pkg.url, resolved_pkg.version);

    package_backup_remove(pkg);
    return resolved_pkg;
}

static void package_backup(const Package& pkg) {
    path_rename(git_modules_path / pkg.path,
                (git_modules_path / pkg.path).string() + backup_suffix);
    path_rename(pkg.path, pkg.path.string() + backup_suffix);
    package_remove(pkg);
}

static void package_backup_remove(const Package& pkg) {
    path_remove((git_modules_path / pkg.path).string() + backup_suffix);
    path_remove(pkg.path.string() + backup_suffix);
}

static void package_backup_restore(const Package& pkg) {
    package_remove(pkg);
    path_rename((git_modules_path / pkg.path).string() + backup_suffix,
                git_modules_path / pkg.path);
    path_rename(pkg.path.string() + backup_suffix, pkg.path);
}

static auto operator>>(std::istream& in, packages::FetchStrategy& strategy) -> std::istream& {
    char raw = '\0';
    if (in >> raw) {
        strategy = static_cast<packages::FetchStrategy>(raw);
    }

    return in;
}

}  // namespace cgen
