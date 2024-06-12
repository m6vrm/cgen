#include <error.hpp>
#include <poost/assert.hpp>

namespace cgen {

auto Error::description() const -> std::string {
    switch (type) {
    case ErrorType::ConfigUnsupportedVersion:
        POOST_ASSERT(!subject.empty(), "empty subject");
        return "unsupported config version: " + subject;
        break;
    case ErrorType::ConfigValidationError:
        POOST_ASSERT(!subject.empty(), "empty subject");
        return "config validation error: " + subject;
        break;
    case ErrorType::ConfigIncludeNotFound:
        POOST_ASSERT(!subject.empty(), "empty subject");
        return "config include file not found: " + subject;
        break;
    case ErrorType::ConfigUndefinedIncludeParameter:
        POOST_ASSERT(!source.empty(), "empty source");
        POOST_ASSERT(!subject.empty(), "empty subject");
        return source + ": undefined config include parameter: " + subject;
        break;
    case ErrorType::ConfigTemplateNotFound:
        POOST_ASSERT(!source.empty(), "empty source");
        POOST_ASSERT(!subject.empty(), "empty subject");
        return source + ": config template not found: " + subject;
        break;
    case ErrorType::ConfigUndefinedTemplateParameter:
        POOST_ASSERT(!source.empty(), "empty source");
        POOST_ASSERT(!subject.empty(), "empty subject");
        return source + ": undefined config template parameter: " + subject;
        break;
    case ErrorType::PackageNotFound:
        POOST_ASSERT(!subject.empty(), "empty subject");
        return "package not found: " + subject;
        break;
    case ErrorType::PackageVersionResolutionError:
        POOST_ASSERT(!source.empty(), "empty source");
        POOST_ASSERT(!subject.empty(), "empty subject");
        return source + ": package version resolution error: " + subject;
        break;
    case ErrorType::PackageFetchError:
        POOST_ASSERT(!source.empty(), "empty source");
        POOST_ASSERT(!subject.empty(), "empty subject");
        return source + ": package fetch error: " + subject;
        break;
    }

    POOST_ASSERT_FAIL("invalid error type: %d", type);
}

} // namespace cgen
