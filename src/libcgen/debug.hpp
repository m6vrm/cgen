#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace cgen {

auto node_dump(const YAML::Node &node) -> std::string;

} // namespace cgen
