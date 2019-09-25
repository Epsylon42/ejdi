#include <ast.hpp>

using namespace std;
using namespace ejdi;
using namespace ejdi::ast;

static string offset(size_t depth) {
    string res;
    for (size_t i = 0; i < depth; i++) {
        res += "  ";
    }
    return res;
}

string ast::Assignment::debug(size_t depth) const {
    string res = offset(depth);

    res += "assignment ";

    if (let.has_value()) {
        res += let->debug();
        res += ' ';
    }

    res += ast_debug(variable);
    res += ' ';
    res += ast_debug(assignment);
    res += ' ';
    res += ast_debug(expr);
    res += ' ';
    res += ast_debug(semi);

    return res;
}


string ast::ExprStmt::debug(size_t depth) const {
    string res = ast_debug(expr, depth);
    res += ' ';
    res += ast_debug(semi);

    return res;
}


string ast::Variable::debug(size_t depth) const {
    string res = offset(depth);

    res += "var(" + ast_debug(variable) + ")";

    return res;
}


string ast::Block::debug(size_t depth) const {
    string res = offset(depth);

    res += "block";

    for (const auto& st : statements) {
        res += '\n';
        res += ast_debug(st, depth + 1);
    }

    if (ret.has_value()) {
        res += '\n';
        res += ast_debug(*ret, depth + 1);
    }

    return res;
}


string ast::ParenExpr::debug(size_t depth) const {
    return "unimplemented";
}

string ast::BinaryOp::debug(size_t depth) const {
    string res = offset(depth);

    res += "binary op ";
    res += get_str(op);

    res += '\n';
    res += ast_debug(left, depth + 1);
    res += '\n';
    res += ast_debug(right, depth + 1);

    return res;
}

string ast::UnaryOp::debug(size_t depth) const {
    string res = offset(depth);

    res += "unary ";
    res += get_str(op);
    res += '\n';
    res += ast_debug(expr, depth + 1);

    return res;
}

string ast::FunctionCall::debug(size_t depth) const {
    string res = offset(depth);

    res += "function call\n";
    res += ast_debug(function, depth + 1);

    if (!arguments->list.empty()) {
        res += '\n';
        res += offset(depth);
        res += "arguments";

        for (const auto& arg : arguments->list) {
            res += '\n';
            res += ast_debug(arg, depth + 1);
        }
    }

    return res;
}

string ast::WhileLoop::debug(size_t depth) const {
    string res = offset(depth);

    res += "while\n";
    res += ast_debug(condition, depth + 1);

    res += '\n';
    res += offset(depth);
    res += "loop\n";
    res += block->debug(depth + 1);

    return res;
}

string ast::IfThenElse::debug(size_t depth) const {
    string res = offset(depth);

    res += "if\n";
    res += ast_debug(condition, depth + 1);

    res += '\n';
    res += offset(depth);
    res += "then\n";
    res += then->debug(depth + 1);

    if (else_.has_value()) {
        res += '\n';
        res += offset(depth);
        res += "else\n";

        res += ast_debug(get<1>(*else_), depth + 1);
    }

    return res;
}
