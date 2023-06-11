#include "fs.hpp"

#include <poost/log.hpp>

namespace cgen {

void path_rename(const std::filesystem::path &path, const std::filesystem::path &new_path) {
    if (!path_is_sub(path, std::filesystem::current_path()) ||
        !path_is_sub(new_path, std::filesystem::current_path())) {

        POOST_FATAL("renaming paths outside of the current working dir is prohibited: %s -> %s",
                    path.c_str(), new_path.c_str());
        return;
    }

    if (std::filesystem::exists(path)) {
        POOST_DEBUG("rename path: %s -> %s", path.c_str(), new_path.c_str());
        std::filesystem::rename(path, new_path);
    }
}

auto path_is_dir(const std::filesystem::path &path) -> bool {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

auto path_is_empty(const std::filesystem::path &path) -> bool {
    return !std::filesystem::exists(path) || std::filesystem::is_empty(path);
}

auto path_is_sub(const std::filesystem::path &path, const std::filesystem::path &base) -> bool {
    const std::filesystem::path cb = std::filesystem::weakly_canonical(base);
    const std::filesystem::path cp = std::filesystem::weakly_canonical(cb / path);

    for (auto ip = cp.begin(), ib = cb.begin(); ib != cb.end(); ip++, ib++) {
        if (ip == cp.end() || *ip != *ib) {
            return false;
        }
    }

    return true;
}

auto path_is_equal(const std::filesystem::path &path1, const std::filesystem::path &path2) -> bool {
    const std::filesystem::path cp1 = std::filesystem::weakly_canonical(path1);
    const std::filesystem::path cp2 = std::filesystem::weakly_canonical(path2);
    return cp1 == cp2;
}

} // namespace cgen
