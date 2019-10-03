#include <ast.hpp>
#include <span.hpp>

using namespace std;
using namespace ejdi;
using namespace ejdi::ast;
using namespace ejdi::span;

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
    }
    res += '\n';

    if (base.has_value()) {
        res += ast_debug(*base, depth + 1);
        res += '\n';
    }

    res += offset(depth);
    res += "field ";
    res += ast_debug(field);
    res += ' ';
    res += ast_debug(assignment);
    res += '\n';
    res += ast_debug(expr, depth + 1);
    res += ' ';
    res += ast_debug(semi);

    return res;
}

Span ast::Assignment::span() const {
    auto res = Span::empty();
    if (let.has_value()) {
        res = let->span;
    } else if (base.has_value()) {
        res = ast_span(*base);
    }
    return res.join(field.span).join(semi.span);
}


string ast::ExprStmt::debug(size_t depth) const {
    string res = ast_debug(expr, depth);
    res += ' ';
    res += ast_debug(semi);

    return res;
}

Span ast::ExprStmt::span() const {
    return ast_span(expr).join(semi.span);
}


string ast::Variable::debug(size_t depth) const {
    string res = offset(depth);

    res += "var(" + ast_debug(variable) + ")";

    return res;
}

Span ast::Variable::span() const {
    return variable.span;
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

Span ast::Block::span() const {
    if (parens.has_value()) {
        return parens->span;
    }
    auto res = Span::empty();
    if (!statements.empty()) {
        res = ast_span(statements.front()).join(ast_span(statements.back()));
    }
    if (ret.has_value()) {
        return res.join(ast_span(*ret));
    } else {
        return res;
    }
}


// string ast::ParenExpr::debug(size_t depth) const {
//     return "unimplemented";
// }

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

Span ast::BinaryOp::span() const {
    return ast_span(left).join(ast_span(right));
}


string ast::UnaryOp::debug(size_t depth) const {
    string res = offset(depth);

    res += "unary ";
    res += get_str(op);
    res += '\n';
    res += ast_debug(expr, depth + 1);

    return res;
}

Span ast::UnaryOp::span() const {
    return op.span.join(ast_span(expr));
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

Span ast::FunctionCall::span() const {
    return ast_span(function).join(arguments->span());
}


string ast::FieldAccess::debug(size_t depth) const {
    string res = offset(depth);

    res += "field access '";
    res += ast_debug(field);
    res += "' on\n";

    res += ast_debug(base, depth + 1);

    return res;
}

Span ast::FieldAccess::span() const {
    return ast_span(base).join(field.span);
}


string ast::MethodCall::debug(size_t depth) const {
    string res = offset(depth);

    res += "method call '";
    res += ast_debug(method);
    res += "' on\n";
    res += ast_debug(base, depth + 1);

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

Span ast::MethodCall::span() const {
    return ast_span(base).join(arguments->span());
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

Span ast::WhileLoop::span() const {
    return while_.span.join(block->span());
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

Span ast::IfThenElse::span() const {
    Span ret = if_.span.join(then->span());
    if (else_.has_value()) {
        return ret.join(ast_span(std::get<1>(*else_)));
    } else {
        return ret;
    }
}


string ast::NumberLiteral::debug(size_t depth) const {
    return offset(depth) + "lit(" + to_string(literal.value()) + ")";
}

Span ast::NumberLiteral::span() const {
    return literal.span;
}


string ast::StringLiteral::debug(size_t depth) const {
    return offset(depth) + "lit(\"" + literal.value() + "\")";
}

Span ast::StringLiteral::span() const {
    return literal.span;
}


string ast::BoolLiteral::debug(size_t depth) const {
    return offset(depth) + "lit(" + (value ? "true" : "false") + ")";
}

Span ast::BoolLiteral::span() const {
    return word.span;
}


template< typename T >
Span ast::List<T>::span() const {
    if (parens.has_value()) {
        return parens->span;
    } else if (!list.empty()) {
        return ast_span(list.front()).join(ast_span(list.back()));
    } else {
        return Span::empty();
    }
}
