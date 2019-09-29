#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser.hpp>
#include <linemap.hpp>

#include <exec/exec.hpp>
#include <exec/value.hpp>

using namespace std;
using namespace ejdi;

int main() {
    auto file = ifstream("test.ejdi");
    string source {istreambuf_iterator<char>(file), {}};

    auto linemap = linemap::Linemap(source);

    auto lexems = lexer::actions::split_string(source, "test.ejdi");
    auto group = lexer::groups::find_groups(move(lexems));

    parser::ParseStream stream(group->inner);
    auto block = parser::parse<ast::Rc<ast::Block>>(stream);
    if (block.has_result()) {
        using namespace ejdi::exec;
        using namespace ejdi::exec::value;
        using namespace ejdi::exec::context;

        auto ctx = GlobalContext::with_core();

        auto mod = ctx.new_module("test");

        ast::Expr expr(move(block.get()));
        ejdi::exec::eval(mod, expr);
    } else {
        auto& error = block.error();
        auto pos = linemap.span_to_pos_pair(error.span).first;
        cout << "ERROR at " << pos.line+1 << ':' << pos.column << endl;
        cout << "  expected " << error.expected
             << " but got " << error.got
             << endl;
    }
}
