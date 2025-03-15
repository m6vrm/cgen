#include <args.hpp>

namespace poost {

Args::Args(int argc, const char* argv[]) : argi_{0}, argc_{argc - 1}, argv_{&argv[1]} {}

char Args::option() {
    while (argc_ > 0) {
        bool is_first = argi_ == 0;
        char current = (*argv_)[argi_];
        argi_++;

        if (is_first) {
            if (current == '-') {
                continue;
            } else {
                return args::not_an_option;
            }
        }

        if (current == '\0') {
            argi_ = 0;
            --argc_;
            ++argv_;
            continue;
        }

        return current;
    }

    return args::end;
}

bool Args::value(const char** result) {
    if (argi_ != 0) {
        if ((*argv_)[argi_] == '\0') {
            argi_ = 0;
            --argc_;
            ++argv_;
        } else {
            return false;
        }
    }

    if (argc_ == 0 || (*argv_)[argi_] == '-') {
        return false;
    }

    *result = *argv_;
    argi_ = 0;
    --argc_;
    ++argv_;
    return true;
}

const char* Args::peek() const {
    return *argv_;
}

}  // namespace poost
