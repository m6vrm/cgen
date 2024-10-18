#pragma once

#include <istream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace poost {

class Config {
   public:
    explicit Config(std::istream& is);

    template <typename T>
    T value(const std::string& key, const T& fallback) {
        for (auto& [config_key, config_value] : config_) {
            if (config_key != key) {
                continue;
            }

            if constexpr (std::is_same_v<T, std::string>) {
                return config_value;
            } else {
                return convert<T>(config_value);
            }
        }

        return fallback;
    }

   private:
    template <typename T>
    T convert(const std::string& string) {
        T result;
        std::istringstream iss{string};
        iss >> result;
        return result;
    }

    std::vector<std::pair<std::string, std::string>> config_;
};

}  // namespace poost
