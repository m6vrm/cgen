#include "git.hpp"
#include "exec.hpp"

#include <poost/assert.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace cgen {

auto git_is_commit(const std::string &str, bool strict) -> bool {
    if (strict && str.size() != 40) {
        return false;
    }

    return std::all_of(str.cbegin(), str.cend(),
                       [](unsigned char c) -> bool { return std::isxdigit(c); });
}

auto git_remote_tags(const std::string &url, std::vector<std::string> &tags) -> int {
    std::string out;
    const int status = exec(out, {"git", "ls-remote", "--tags", "--refs", url});
    if (status != EXIT_SUCCESS) {
        return status;
    }

    std::stringstream ss{out};
    std::string line;
    while (std::getline(ss, line)) {
        const std::string prefix = "refs/tags/";
        const std::string tag = line.substr(line.find(prefix) + prefix.size());
        tags.push_back(tag);
    }

    return status;
}

auto git_resolve_ref(const std::filesystem::path &repo, const std::string &ref, std::string &commit)
    -> int {

    std::string out;
    const int status = exec(out, {"git", "-C", repo, "rev-parse", "--verify", ref});
    if (status != EXIT_SUCCESS) {
        return status;
    }

    POOST_ASSERT(git_is_commit(out), "invalid commit hash: %s", out.c_str());

    commit = out;
    return status;
}

auto git_reset_hard(const std::filesystem::path &repo, const std::string &ref) -> int {
    std::string out;
    const int status = exec(out, {"git", "-C", repo, "reset", "--hard", ref});
    return status;
}

auto git_remove(const std::filesystem::path &path) -> int {
    std::string out;
    const int status = exec(out, {"git", "rm", "--force", "--ignore-unmatch", path});
    return status;
}

auto git_clone_shallow(const std::filesystem::path &path, const std::string &url) -> int {
    std::string out;
    const int status = exec(out, {"git", "clone", "--recursive", "--depth 1", url, path});
    return status;
}

auto git_clone_full(const std::filesystem::path &path, const std::string &url) -> int {
    std::string out;
    int status = exec(out, {"git", "clone", "--recursive", url, path});
    return status;
}

auto git_clone_branch(const std::filesystem::path &path, const std::string &url,
                      const std::string &branch) -> int {

    std::string out;
    const int status =
        exec(out, {"git", "clone", "--recursive", "--depth 1", "--branch", branch, url, path});
    return status;
}

auto git_submodule_add(const std::filesystem::path &path, const std::string &url) -> int {
    std::string out;
    const int status = exec(out, {"git", "submodule", "add", "--force", url, path});
    return status;
}

auto git_submodule_init(const std::filesystem::path &path) -> int {
    std::string out;
    const int status =
        exec(out, {"git", "-C", path, "submodule", "update", "--init", "--recursive"});
    return status;
}

auto git_submodule_deinit(const std::filesystem::path &path) -> int {
    std::string out;
    const int status = exec(out, {"git", "submodule", "deinit", "--force", path});
    return status;
}

} // namespace cgen
