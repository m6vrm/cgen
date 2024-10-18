#pragma once

#include <fmt/format.h>
#include <filesystem>
#include <string_view>
#include <type_traits>

template <typename EnumType>
    requires std::is_enum_v<EnumType>
struct fmt::formatter<EnumType> : fmt::formatter<std::underlying_type_t<EnumType>> {
    auto format(const EnumType& value, fmt::format_context& context) const {
        return fmt::formatter<std::underlying_type_t<EnumType>>::format(
            static_cast<std::underlying_type_t<EnumType>>(value), context);
    }
};

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string_view> {
    auto format(const std::filesystem::path& path, fmt::format_context& context) const {
        return formatter<std::string_view>::format(path.string(), context);
    }
};
