#pragma once

#include <initializer_list>
#include <string>

namespace cgen {

auto exec(std::string &out,
          std::initializer_list<std::string> cmd_parts) -> int;

} // namespace cgen
