#pragma once

#include <filesystem>
#include <iostream>

namespace cgen {

auto path_exists(const std::filesystem::path& path) -> bool;
void path_remove(const std::filesystem::path& path);
void path_rename(const std::filesystem::path& path, const std::filesystem::path& new_path);

auto path_is_dir(const std::filesystem::path& path) -> bool;
auto path_is_empty(const std::filesystem::path& path) -> bool;
auto path_is_sub(const std::filesystem::path& path, const std::filesystem::path& base) -> bool;
auto path_is_equal(const std::filesystem::path& path1, const std::filesystem::path& path2) -> bool;

void file_read(const std::filesystem::path& path, std::istream& in);

}  // namespace cgen
