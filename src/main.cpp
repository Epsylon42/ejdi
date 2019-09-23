#include <iostream>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser.hpp>

using namespace ejdi;

int main() {
    auto example = "{ let a = b; a + b * c; c; { hello } }";
    // auto example = "(1 [+] 1) 5";

    auto lexems = lexer::actions::split_string(example, "");
    auto group = lexer::groups::find_groups(move(lexems));

    parser::ParseStream stream(group->inner);
    auto expr = parser::parse<std::shared_ptr<ast::Block>>(stream);
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
