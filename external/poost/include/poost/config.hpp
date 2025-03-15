#pragma once

#include <filesystem>
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

    static Config load(const std::filesystem::path& path);

    template <typename T>
    T get(const std::string& at, const T& fallback) const {
        for (auto& [key, value] : config_) {
            if (key != at) {
                continue;
            }

            if constexpr (std::is_same_v<T, std::string>) {
                return value;
            } else {
                return convert<T>(value);
            }
        }

        return fallback;
    }

   private:
    template <typename T>
    T convert(const std::string& string) const {
        T result;
        std::istringstream iss{string};
        iss >> result;
        return result;
    }

    std::vector<std::pair<std::string, std::string>> config_;
};

}  // namespace poost
