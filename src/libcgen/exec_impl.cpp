#include "exec.hpp"

#include <poost/log.hpp>

#include <array>
#include <cstdlib>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif // ifdef _WIN32

namespace cgen {

// todo: consider exec(3) family of functions
auto exec(std::string &out, std::initializer_list<std::string> cmd_parts) -> int {
    std::string cmd;
    for (const std::string &part : cmd_parts) {
        cmd += part + " ";
    }

    POOST_TRACE("execute command: %s", cmd.c_str());
    std::FILE *fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        POOST_ERROR("can't create pipe: %s", cmd.c_str());
        return -1;
    }

    std::array<char, 8192> buf;
    std::size_t len;
    do {
        len = std::fread(buf.data(), sizeof(buf[0]), buf.size(), fp);
        out += std::string{buf.data(), len};
    } while (len > 0);

    // trim trailing newline
    if (!out.empty() && out.back() == '\n') {
        out.pop_back();
    }

    int status = pclose(fp);
    // cppcheck-suppress selfAssignment
    status = WEXITSTATUS(status);

    if (status != EXIT_SUCCESS) {
        POOST_WARN("command failed: %s"
                   "\n\texit status: %d",
                   cmd.c_str(), status);
    }

    return status;
}

} // namespace cgen
