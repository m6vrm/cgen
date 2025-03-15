#include <algorithm>
#include <cctype>
#include <limits>
#include <versions.hpp>

namespace cgen {

struct Version {
    std::vector<int> normal;
    std::vector<int> rc;
    std::vector<int> build;
    bool has_rc;
};

static auto version_parse(const std::string& ver) -> Version;

/// Public

auto version_is_valid(const std::string& ver) -> bool {
    for (const unsigned char c : ver) {
        if (c != '.' && c != '*' && !std::isdigit(c)) {
            return false;
        }
    }

    return true;
}

auto version_match(const std::string& ver, const std::string& tag, bool ignore_rc) -> bool {
    // use only normal parts for matching
    const Version tag_ver = version_parse(tag);
    const std::vector<int> ver_parts = version_parse(ver).normal;
    const std::vector<int>& tag_parts = tag_ver.normal;

    if (ignore_rc && tag_ver.has_rc) {
        // ignore pre-releases
        return false;
    }

    if (ver_parts == tag_parts) {
        // success if version is equal to tag
        return true;
    }

    for (std::size_t i = 0; i < ver_parts.size(); ++i) {
        if (ver_parts[i] == std::numeric_limits<int>::max()) {
            if (i == ver_parts.size() - 1) {
                // success if last part is wildcard
                return true;
            } else {
                // skip wildcard
                continue;
            }
        } else if (i >= tag_parts.size()) {
            if (ver_parts[i] == 0) {
                // skip zero overflow
                continue;
            } else {
                // failure on overflow
                return false;
            }
        } else if (ver_parts[i] != tag_parts[i]) {
            // failure on mismatch
            return false;
        }
    }

    // failure if there's more tag parts and version doesn't end with wildcard
    return ver_parts.size() >= tag_parts.size();
}

auto version_less(const std::string& lhs, const std::string& rhs) -> bool {
    // see https://semver.org/spec/v2.0.0-rc.1.html
    Version lhs_parts = version_parse(lhs);
    Version rhs_parts = version_parse(rhs);

    if (lhs_parts.normal != rhs_parts.normal) {
        // compare by normal parts
        return std::lexicographical_compare(lhs_parts.normal.cbegin(), lhs_parts.normal.cend(),
                                            rhs_parts.normal.cbegin(), rhs_parts.normal.cend());
    } else if (lhs_parts.has_rc && !rhs_parts.has_rc) {
        // prefer version without pre-release parts
        return true;
    } else if (!lhs_parts.has_rc && rhs_parts.has_rc) {
        // prefer version without pre-release parts
        return false;
    } else if (lhs_parts.rc != rhs_parts.rc) {
        // compare by pre-release parts
        return std::lexicographical_compare(lhs_parts.rc.cbegin(), lhs_parts.rc.cend(),
                                            rhs_parts.rc.cbegin(), rhs_parts.rc.cend());
    } else if (lhs_parts.build != rhs_parts.build) {
        // compare by build parts
        // note: not according to the semver spec where build version has higher
        // precedence even than the associated normal version
        return std::lexicographical_compare(lhs_parts.build.cbegin(), lhs_parts.build.cend(),
                                            rhs_parts.build.cbegin(), rhs_parts.build.cend());
    }

    // prefer longest version (prefixed or with more parts)
    return lhs.size() < rhs.size();
}

auto version_tag(const std::string& ver, const std::vector<std::string>& tags, bool ignore_rc)
    -> std::optional<std::string> {
    std::vector<std::string> sorted_tags{tags};
    std::sort(sorted_tags.rbegin(), sorted_tags.rend(), version_less);

    for (auto it = sorted_tags.cbegin(); it != sorted_tags.cend(); ++it) {
        const std::string tag = *it;

        if (version_match(ver, tag, ignore_rc)) {
            return std::optional<std::string>{tag};
        }
    }

    return std::nullopt;
}

/// Private

static void vector_remove_trailing_zeros(std::vector<int>& vector) {
    const auto zeroes_it =
        std::find_if(vector.crbegin(), vector.crend(), [](int val) -> bool { return val != 0; });
    vector.erase(zeroes_it.base(), vector.end());
}

static auto version_parse(const std::string& ver) -> Version {
    Version result{};

    enum {
        ST_NORMAL,
        ST_RC,
        ST_BUILD,
    } state = ST_NORMAL;

    std::vector<int>* parts = &result.normal;

    for (auto it = ver.cbegin(); it != ver.cend(); ++it) {
        const unsigned char c = *it;

        if (std::isdigit(c)) {
            // parse number
            std::string::size_type next_idx = 0;
            const int part = std::stoi(&(*it), &next_idx);
            parts->push_back(part);
            it += next_idx - 1;
        } else if (c == '*') {
            // parse wildcard
            parts->push_back(std::numeric_limits<int>::max());
        } else if (state == ST_NORMAL && c == '-') {
            // switch to pre-release parts
            state = ST_RC;
            result.has_rc = true;
            parts = &result.rc;
        } else if ((state == ST_NORMAL || state == ST_RC) && c == '+') {
            // switch to build parts
            state = ST_BUILD;
            parts = &result.build;
        }
    }

    vector_remove_trailing_zeros(result.normal);
    vector_remove_trailing_zeros(result.rc);
    vector_remove_trailing_zeros(result.build);

    return result;
}

}  // namespace cgen
