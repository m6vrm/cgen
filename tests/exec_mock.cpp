#include <cstdlib>
#include <exec.hpp>
#include <mocks.hpp>

namespace cgen {

std::map<std::string, std::string> outputs;

void mock_exec(const std::map<std::string, std::string> &mocks) {
    outputs = mocks;
}

auto exec(std::string &out,
          std::initializer_list<std::string> cmd_parts) -> int {
    std::string cmd;
    for (const std::string &part : cmd_parts) {
        if (&part != cmd_parts.end()) {
            cmd += part + " ";
        } else {
            cmd += part;
        }
    }

    out = outputs[cmd];

    return EXIT_SUCCESS;
}

} // namespace cgen
