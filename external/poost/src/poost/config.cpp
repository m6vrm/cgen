#include <config.hpp>
#include <iomanip>

namespace poost {

Config::Config(std::istream& is) {
    std::string line;
    while (std::getline(is, line)) {
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

}  // namespace poost
