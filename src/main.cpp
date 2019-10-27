#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include <exec/context.hpp>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "not enough arguments" << endl;
        return 1;
    }

    auto ctx = ejdi::exec::context::GlobalContext::with_core();
    try {
        ctx.load_module(argv[1]);
    } catch (ejdi::exec::error::RuntimeError& e) {
        ctx.print_error_message(e);
    }

    // auto file = ifstream(argv[1]);
    // string source {istreambuf_iterator<char>(file), {}};

    // auto linemap = linemap::Linemap(source);

    // auto lexems = lexer::actions::split_string(source, "test.ejdi");

    // auto group = lexer::groups::find_groups(move(lexems));

    // auto program = parser::parse_program(*group);
    // if (program.has_result()) {
    //     using namespace ejdi::exec;
    //     using namespace ejdi::exec::value;
    //     using namespace ejdi::exec::context;

    //     auto ctx = GlobalContext::with_core();

    //     auto mod = ctx.new_module("test");

    //     try {
    //         ejdi::exec::exec_program(mod, *program.get());
    //     } catch (error::RuntimeError& e) {
    //         auto pos = linemap.span_to_pos_pair(e.root_span).first;
    //         cerr << "RUNTIME ERROR at " << pos.line+1 << ':' << pos.column << endl;
    //         cerr << "  " << e.root_error << endl;
    //     }
    // } else {
    //     auto& error = program.error();
    //     auto pos = linemap.span_to_pos_pair(error.span).first;
    //     cerr << "ERROR at " << pos.line+1 << ':' << pos.column << endl;
    //     cerr << "  expected " << error.expected
    //          << " but got " << error.got
    //          << endl;
    // }
}
