#pragma once

#include <string_view>

namespace ejdi::util {
    bool starts_with(std::string_view str, std::string_view pat);
    bool starts_with(std::string_view str, char pat);
}
