#ifndef CGEN_DEBUG_HPP
#define CGEN_DEBUG_HPP

#include <yaml-cpp/yaml.h>

#include <string>

namespace cgen {

auto node_dump(const YAML::Node &node) -> std::string;

} // namespace cgen

#endif // ifndef CGEN_DEBUG_HPP
