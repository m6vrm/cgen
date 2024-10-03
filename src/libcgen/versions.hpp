#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cgen {

auto version_is_valid(const std::string &ver) -> bool;
auto version_match(const std::string &ver, const std::string &tag, bool ignore_rc = false) -> bool;
auto version_less(const std::string &lhs, const std::string &rhs) -> bool;
auto version_tag(const std::string &ver, const std::vector<std::string> &tags,
                 bool ignore_rc = false) -> std::optional<std::string>;

} // namespace cgen
