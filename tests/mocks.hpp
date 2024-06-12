#pragma once

#include <filesystem>
#include <map>
#include <string>

namespace cgen {

void mock_files(const std::map<std::filesystem::path, std::string> &mocks);
void mock_exec(const std::map<std::string, std::string> &mocks);

} // namespace cgen
