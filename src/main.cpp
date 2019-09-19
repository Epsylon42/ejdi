#include <iostream>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>

using namespace ejdi;

int main() {
    auto example = "  hello world + abce; a += (1.5 + 6); \"Hello, World!\" ";
    // auto example = "(1 [+] 1) 5";

    auto lexems = lexer::actions::split_string(example, "");

    auto grouped = lexer::groups::find_groups(move(lexems));
    std::cout << grouped->debug() << std::endl;
}
