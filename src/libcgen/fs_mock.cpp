#include "fs.hpp"
#include "mocks.hpp"

#include <sstream>

namespace cgen {

std::map<std::filesystem::path, std::string> files;
std::stringstream ss;

void mock_files(const std::map<std::filesystem::path, std::string> &mocks) { files = mocks; }

auto path_exists(const std::filesystem::path &path) -> bool { return files.contains(path); }
void path_remove(const std::filesystem::path &path) { files.erase(path); }

void file_read(const std::filesystem::path &path, std::istream &in) {
    const std::string content = files.at(path);
    ss << content;
    in.rdbuf(ss.rdbuf());
}

} // namespace cgen
