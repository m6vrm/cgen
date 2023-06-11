#ifndef CGEN_ERROR_HPP
#define CGEN_ERROR_HPP

#include <string>

namespace cgen {

enum class ErrorType {
    ConfigUnsupportedVersion,
    ConfigValidationError,
    ConfigIncludeNotFound,
    ConfigUndefinedIncludeParameter,
    ConfigTemplateNotFound,
    ConfigUndefinedTemplateParameter,

    PackageNotFound,
    PackageVersionResolutionError,
    PackageFetchError,
};

struct Error {
    ErrorType type;
    std::string source;
    std::string subject;

    auto description() const -> std::string;
};

} // namespace cgen

#endif // ifndef CGEN_ERROR_HPP
