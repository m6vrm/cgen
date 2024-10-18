#include <args.hpp>

namespace poost {

char Args::option() {
    while (has_args()) {
        bool is_first = is_first_char();
        char next = next_char();

        if (is_first) {
            if (next == '-') {
                // option group start
                // move to the next char
                continue;
            } else {
                // not an option group
                // return with error
                return args::not_an_option;
            }
        }

        if (next == '\0') {
            // option group end
            // move to the next arg
            skip_arg();
            continue;
        }

        // return parsed option
        return next;
    }

    // arg list end
    return args::end;
}

bool Args::value(const char** result) {
    if (!is_first_char()) {
        // inside option group
        if (peek_char() == '\0') {
            // option group end
            // move to the next arg
            skip_arg();
        } else {
            // not option group end
            // forbid value parsing
            return false;
        }
    }

    if (!has_args() || peek_char() == '-') {
        // arg list end or option group start
        // forbid value parsing
        return false;
    }

    // parse value and move to the next arg
    *result = *argv_;
    skip_arg();
    return true;
}

const char* Args::peek() const {
    return *argv_;
}

bool Args::is_first_char() const {
    return argi_ == 0;
}
char Args::peek_char() const {
    return (*argv_)[argi_];
}
char Args::next_char() {
    return (*argv_)[argi_++];
}

bool Args::has_args() const {
    return argc_ > 0;
}

void Args::skip_arg() {
    argi_ = 0;
    --argc_;
    ++argv_;
}

}  // namespace poost
