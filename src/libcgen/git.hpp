#ifndef CGEN_GIT_HPP
#define CGEN_GIT_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace cgen {

auto git_is_commit(const std::string &str, bool strict = true) -> bool;

auto git_remote_tags(const std::string &url, std::vector<std::string> &tags) -> int;
auto git_resolve_ref(const std::filesystem::path &repo, const std::string &ref, std::string &commit)
    -> int;
auto git_reset_hard(const std::filesystem::path &repo, const std::string &ref) -> int;
auto git_remove(const std::filesystem::path &path) -> int;

auto git_clone_shallow(const std::filesystem::path &path, const std::string &url) -> int;
auto git_clone_full(const std::filesystem::path &path, const std::string &url) -> int;
auto git_clone_branch(const std::filesystem::path &path, const std::string &url,
                      const std::string &branch) -> int;

auto git_submodule_add(const std::filesystem::path &path, const std::string &url) -> int;
auto git_submodule_init(const std::filesystem::path &path) -> int;
auto git_submodule_deinit(const std::filesystem::path &path) -> int;

} // namespace cgen

#endif // ifndef CGEN_GIT_HPP
