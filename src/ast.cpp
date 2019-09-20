#include <ast.hpp>

using namespace std;
using namespace ejdi;

string ast::Assignment::debug(size_t depth) const {
    string res;
    for (size_t i = 0; i < depth; i++) {
        res += "  ";
    }

    res += "assignment ";

    if (let.has_value()) {
        res += let->debug();
        res += ' ';
    }

    res += variable.debug();
    res += ' ';
    res += assignment.debug();
    res += ' ';
    res += expr->debug();
    res += ' ';
    res += semi.debug();

    return res;
}


string ast::ExprStmt::debug(size_t depth) const {
    string res;
    for (size_t i = 0; i < depth; i++) {
        res += "  ";
    }

    res += expr->debug();
    res += ' ';
    res += semi.debug();

    return res;
}


string ast::Variable::debug(size_t depth) const {
    string res;
    for (size_t i = 0; i < depth; i++) {
        res += "  ";
    }

    res += "var(" + variable.debug() + ")";

    return res;
}


string ast::Block::debug(size_t depth) const {
    string res;
    for (size_t i = 0; i < depth; i++) {
        res += "  ";
    }

    res += "block";

    for (const auto& st : statements) {
        res += '\n';
        res += st->debug(depth + 1);
    }

    if (ret.has_value()) {
        res += '\n';
        res += (*ret)->debug(depth + 1);
    }

    return res;
}


string ast::Paren::debug(size_t depth) const {
    return "unimplemented";
}
