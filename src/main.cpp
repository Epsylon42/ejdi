#include <iostream>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <parser.hpp>

using namespace ejdi;

int main() {
    auto example = "{ let a = b; b; c; { hello } }";
    // auto example = "(1 [+] 1) 5";

    auto lexems = lexer::actions::split_string(example, "");
    auto grouped = lexer::groups::find_groups(move(lexems));

    parser::ParseStream stream(grouped->inner);
    auto expr = parser::parse<ast::Block>(stream);
    if (expr.has_result()) {
        std::cout << expr.get()->debug() << std::endl;
    } else {
        auto& error = expr.error();
        std::cout << "at " << error.span.start
                  << " expected " << error.expected
                  << " but got " << error.got
                  << std::endl;
    }
}
