#include <fs.hpp>
#include <poost/log.hpp>
#include <fstream>

namespace cgen {

std::ifstream fs;

auto path_exists(const std::filesystem::path &path) -> bool {
    return std::filesystem::exists(path);
}

void path_remove(const std::filesystem::path &path) {
    if (!path_is_sub(path, std::filesystem::current_path())) {
        POOST_FATAL("removing paths outside of the current working dir is "
                    "prohibited: %s",
                    path.c_str());
        return;
    }

    POOST_DEBUG("remove everything at path: %s", path.c_str());
    std::filesystem::remove_all(path);
}

void file_read(const std::filesystem::path &path, std::istream &in) {
    if (fs.is_open()) {
        fs.close();
        fs.clear();
    }

    fs.open(path, std::ifstream::in);
    in.rdbuf(fs.rdbuf());
}

} // namespace cgen
