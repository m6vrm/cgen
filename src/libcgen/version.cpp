#include <version.hpp>

namespace cgen {

auto version_string() -> std::string {
    // can't concat constexpr strings at compile time without ton of ugly C++ code
    return std::to_string(version::major) + "." + std::to_string(version::minor) + "." +
           std::to_string(version::patch);
}

}  // namespace cgen
