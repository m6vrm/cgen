#include "config.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <log.hpp>

namespace poost {

Config::Config(std::istream& is) {
    std::string line;
    while (std::getline(is, line)) {
        // ignore comments
        std::size_t found = line.find("#");
        if (found != std::string::npos) {
            line.erase(found);
        }

        std::istringstream iss{line};
        std::string key;
        std::string value;
        if (iss >> key >> std::quoted(value)) {
            config_.emplace_back(key, value);
        }
    }
}

Config Config::load(const std::filesystem::path& path) {
    std::ifstream config_ifs{path};
    if (!config_ifs) {
        POOST_WARN("missing config: {}", path);
    }

    return Config{config_ifs};
}

}  // namespace kerrad::core
