#include <iostream>

#include <span.hpp>
#include <lexer.hpp>

using namespace ejdi;

int main() {
    auto example = "  hello world + abce; a += b ";
    auto res = lexer::actions::split_string(example, "");

    for (auto& lexem : res) {
        std::cout << lexem->str << std::endl;
    }
}
