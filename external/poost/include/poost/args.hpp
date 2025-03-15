#pragma once

#include <sstream>

namespace poost {

namespace args {

inline constexpr char end = -1;
inline constexpr char not_an_option = -2;

}  // namespace args

class Args {
   public:
    Args(int argc, const char* argv[]);

    char option();
    bool value(const char** result);
    const char* peek() const;

    template <typename T>
    bool value(T& result) {
        const char* string;
        if (value(&string)) {
            std::istringstream iss{string};
            iss >> result;
            return true;
        }

        return false;
    }

   private:
    int argi_;
    int argc_;
    const char** argv_;
};

}  // namespace poost
