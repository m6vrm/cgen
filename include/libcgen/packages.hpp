#ifndef CGEN_PACKAGES_HPP
#define CGEN_PACKAGES_HPP

#include "error.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace cgen {

namespace packages {

enum class FetchStrategy : char {
    Submodule = 's',
    Clone = 'c',
};

} // namespace packages

struct Package {
    packages::FetchStrategy strategy;
    std::filesystem::path path;
    std::string url;
    std::string version;
    std::string original_version;
};

auto packages_cleanup(const std::vector<Package> &pkgs, const std::vector<Package> &resolved_pkgs)
    -> std::vector<Package>;
auto packages_resolve(const std::vector<Package> &pkgs, const std::vector<Package> &resolved_pkgs,
                      std::vector<Error> &errors) -> std::vector<Package>;
auto packages_update(const std::vector<Package> &pkgs,
                     const std::vector<std::filesystem::path> &paths, std::vector<Error> &errors)
    -> std::vector<Package>;

auto packages_merge(const std::vector<Package> &from, const std::vector<Package> &to)
    -> std::vector<Package>;

auto resolved_read(std::istream &in) -> std::vector<Package>;
void resolved_write(std::ostream &out, const std::vector<Package> &resolved_pkgs);

} // namespace cgen

#endif // ifndef CGEN_PACKAGES_HPP
