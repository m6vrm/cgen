#pragma once

#include <yaml-cpp/yaml.h>
#include <string>

namespace cgen {

auto node_dump(const YAML::Node& node) -> std::string;

}  // namespace cgen
