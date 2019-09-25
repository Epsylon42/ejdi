#include <iostream>
#include <fstream>
#include <string>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser.hpp>

using namespace std;
using namespace ejdi;

int main() {
    auto file = ifstream("test.ejdi");
    string source {istreambuf_iterator<char>(file), {}};

    auto lexems = lexer::actions::split_string(source, "test.ejdi");
    auto group = lexer::groups::find_groups(move(lexems));

    parser::ParseStream stream(group->inner);
    auto block = parser::parse<ast::Rc<ast::Block>>(stream);
    if (block.has_result()) {
        std::cout << block.get()->debug() << std::endl;
    } else {
        auto& error = block.error();
        std::cout << "at " << error.span.start
                  << " expected " << error.expected
                  << " but got " << error.got
                  << std::endl;
    }
}
