#include <util.hpp>

using namespace std;

namespace ejdi::util {
    bool starts_with(string_view str, string_view pat) {
        return str.length() >= pat.length()
            && str.substr(0, pat.length()) == pat;
    }

    bool starts_with(string_view str, char c) {
        return !str.empty() && str[0] == c;
    }
}
