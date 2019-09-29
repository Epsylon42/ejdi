#include <algorithm>
#include <cassert>

#include <parser.hpp>

using namespace std;
using namespace ejdi;
using namespace ejdi::lexer;
using namespace ejdi::lexer::groups;
using namespace ejdi::parser;
using namespace ejdi::parser::result;
using namespace ejdi::ast;
using ejdi::span::Span;

ParseStream ParseStream::clone() const {
    return *this;
}

bool ParseStream::peek(string_view str) const {
    if (is_empty()) {
        return false;
    }

    return get_str(*begin) == str;
}

bool ParseStream::is_empty() const {
    return begin >= end;
}

Span ParseStream::span() const {
    if (is_empty()) {
        if (parent == nullptr || !parent->surrounding.has_value()) {
            return Span::empty();
        } else {
            return parent->surrounding->cl.span;
        }
    } else {
        return get_span(*begin);
    }
}

string ParseStream::str() const {
    if (is_empty()) {
        if (parent == nullptr || !parent->surrounding.has_value()) {
            return "";
        } else {
            return parent->surrounding->cl.str;
        }
    } else {
        return string(get_str(*begin));
    }
}


unique_ptr<ParserError> ParseStream::expected(string expected) const {
    return make_unique<ParserError>(span(), move(expected), str());
}


ParserResult<Expr> parser::parse_primary_expr(ParseStream& in) {
    auto stream = in.clone();

    auto num = DO(parse<Rc<NumberLiteral>>(stream));
    if (num.has_result()) {
        in = stream;
        return Expr(move(num.get()));
    }

    auto str = DO(parse<Rc<StringLiteral>>(stream));
    if (str.has_result()) {
        in = stream;
        return Expr(move(str.get()));
    }

    auto boolean = DO(parse<Rc<BoolLiteral>>(stream));
    if (boolean.has_result()) {
        in = stream;
        return Expr(move(boolean.get()));
    }

    auto block = DO(parse<Rc<Block>>(stream));
    if (block.has_result()) {
        in = stream;
        return Expr(move(block.get()));
    }

    auto if_ = DO(parse<Rc<IfThenElse>>(stream));
    if (if_.has_result()) {
        in = stream;
        return Expr(move(if_.get()));
    }

    auto while_ = DO(parse<Rc<WhileLoop>>(stream));
    if (while_.has_result()) {
        in = stream;
        return Expr(move(while_.get()));
    }

    auto word = DO(parse<Word>(stream));
    if (word.has_result()) {
        in = stream;
        return Expr(make_shared<Variable>(Variable{ word.get() }));
    }

    return in.expected("primary expression");
}

ParserResult<Expr> parser::parse_unary_expr(ParseStream& in) {
    const auto valid_ops = { "!", "+", "-" };

    auto stream = in.clone();

    vector<Punct> ops;
    while (true) {
        if (any_of(
                valid_ops.begin(),
                valid_ops.end(),
                [&](auto valid_op){ return stream.peek(valid_op); }
            )
        ) {
            ops.push_back(DO(parse<Punct>(stream)).get());
        } else {
            break;
        }
    }

    auto access = TRY(parse_access_expr(stream));

    while (!ops.empty()) {
        access = make_shared<UnaryOp>(UnaryOp { move(ops.back()), move(access) });
        ops.pop_back();
    }

    in = stream;

    return access;
}

ParserResult<Expr> parser::parse_access_expr(ParseStream& in) {
    auto stream = in.clone();

    auto primary = TRY(parse_primary_expr(stream));

    while (true) {
        if (stream.peek(".")) {
            auto dot = TRY(parse<lexer::Punct>(stream, "."));
            auto field = TRY_CRITICAL(parse<lexer::Word>(stream));

            auto args = DO(parse_list<Expr>(stream, "()"));

            if (args.has_result()) {
                primary = make_shared<MethodCall>(
                    MethodCall {
                        move(primary),
                        dot,
                        field,
                        move(args.get())
                    });
            } else {
                primary = make_shared<FieldAccess>(
                    FieldAccess {
                        move(primary),
                        dot,
                        field
                    });
            }
        } else {
            auto args = DO(parse_list<Expr>(stream, "()"));
            if (args.has_result()) {
                primary = make_shared<FunctionCall>(
                    FunctionCall {
                        move(primary),
                        move(args.get())
                    });
            } else {
                break;
            }
        }
    }

    in = stream;

    return primary;
}

template<>
ParserResult<Expr> parser::parse<Expr>(ParseStream& in) {
    const auto valid_ops = { "==", "<=", ">=", "<", ">", "+", "-", "*", "/", "%", "~", "&&", "||" };

    auto left = TRY(parse_unary_expr(in));

    if (any_of(
            valid_ops.begin(),
            valid_ops.end(),
            [&](auto valid_op){ return in.peek(valid_op); }
            )
        ) {
        auto stream = in;
        auto op = TRY(parse<Punct>(stream));
        auto right = TRY(parse<Expr>(stream));

        in = stream;
        return Expr(
            make_shared<BinaryOp>(
                BinaryOp { move(op), move(left), move(right) }
                )
            );
    } else {
        return Expr(move(left));
    }
}

template<>
ParserResult<Rc<Assignment>> parser::parse<Rc<Assignment>>(ParseStream& in) {
    auto stream = in.clone();

    auto let = try_parse<Word>(stream, "let");

    optional<Expr> base;
    optional<Word> field;

    auto dest_stream = stream.clone();
    auto destination = TRY(parse_access_expr(dest_stream));
    if (ast_is<Variable>(destination)) {
        field = ast_get<Variable>(destination)->variable;
    } else if (ast_is<FieldAccess>(destination)) {
        auto access = ast_get<FieldAccess>(destination);
        base = move(access->base);
        field = access->field;
    } else {
        return stream.expected("valid lvalue expression");
    }
    stream = dest_stream;

    auto assignment = TRY(parse<Punct>(stream, "="));
    auto expr = TRY_CRITICAL(parse<Expr>(stream));
    auto semi = TRY_CRITICAL(parse<Punct>(stream, ";"));

    in = stream;

    return make_shared<Assignment>(Assignment {
        let,
        move(base),
        *field,
        assignment,
        move(expr),
        semi
    });
}

template<>
ParserResult<Rc<ExprStmt>> parser::parse<Rc<ExprStmt>>(ParseStream& in) {
    auto stream = in.clone();

    auto expr = TRY(parse<Expr>(stream));
    auto semi = TRY(parse<Punct>(stream, ";"));

    in = stream;

    return make_shared<ExprStmt>(ExprStmt{ move(expr), semi });
}

template<>
ParserResult<Stmt> parser::parse<Stmt>(ParseStream& in) {
    //TODO: improve error handling
    auto stream = in.clone();
    auto assignment = DO(parse<Rc<Assignment>>(stream));
    if (assignment.has_result()) {
        in = stream;
        return Stmt(move(assignment.get()));
    }

    stream = in.clone();

    auto expr_stmt = DO(parse<Rc<ExprStmt>>(stream));
    if (expr_stmt.has_result()) {
        in = stream;
        return Stmt(move(expr_stmt.get()));
    }

    return in.expected("statement");
}

template<>
ParserResult<Rc<Block>> parser::parse<Rc<Block>>(ParseStream& in) {
    auto stream = in.clone();
    auto group = TRY(parse<Rc<Group>>(stream));
    auto parens = group->surrounding;

    vector<Stmt> statements;
    ParseStream group_stream(*group);
    while (true) {
        if (group_stream.is_empty()) {
            break;
        }

        auto group_stream_clone = group_stream.clone();
        auto stmt = DO(parse<Stmt>(group_stream_clone));
        if (stmt.has_result()) {
            statements.push_back(stmt.get());
            group_stream = group_stream_clone;
        } else {
            auto expr = TRY(parse<Expr>(group_stream));
            if (!group_stream.is_empty()) {
                return group_stream.expected("; or }");
            }

            in = stream;
            return make_shared<Block>(Block{ move(parens), move(statements), move(expr) });
        }
    }

    in = stream;
    return make_shared<Block>(Block{ move(parens), move(statements), nullopt });
}

template<>
ParserResult<Rc<WhileLoop>> parser::parse<Rc<WhileLoop>>(ParseStream& in) {
    auto stream = in.clone();

    auto while_ = TRY(parse<Word>(stream, "while"));
    auto cond = TRY(parse<Expr>(stream));
    auto block = TRY(parse<Rc<Block>>(stream));

    in = stream;

    return make_shared<WhileLoop>(
        WhileLoop {
            while_,
            move(cond),
            move(block),
        }
    );
}

template<>
ParserResult<Rc<IfThenElse>> parser::parse<Rc<IfThenElse>>(ParseStream& in) {
    auto stream = in.clone();

    auto if_ = TRY(parse<Word>(stream, "if"));
    auto cond = TRY(parse<Expr>(stream));
    auto then = TRY(parse<Rc<Block>>(stream));

    optional<tuple<Word, Expr>> else_;
    if (stream.peek("else")) {
        auto else_word = TRY(parse<Word>(stream, "else"));
        auto else_expr = TRY(parse<Expr>(stream));

        else_ = make_tuple(else_word, move(else_expr));
    }

    in = stream;

    return make_shared<IfThenElse>(
        IfThenElse {
            if_,
            move(cond),
            move(then),
            move(else_)
        }
    );
}

template<>
ParserResult<Rc<NumberLiteral>> parser::parse<Rc<NumberLiteral>>(ParseStream& in) {
    auto lit = TRY(parse<lexer::NumberLit>(in));
    return make_shared<NumberLiteral>(NumberLiteral { lit });
}

template<>
ParserResult<Rc<StringLiteral>> parser::parse<Rc<StringLiteral>>(ParseStream& in) {
    auto lit = TRY(parse<lexer::StringLit>(in));
    return make_shared<StringLiteral>(StringLiteral { lit });
}

template<>
ParserResult<Rc<BoolLiteral>> parser::parse<Rc<BoolLiteral>>(ParseStream& in) {
    if (in.peek("true")) {
        return make_shared<BoolLiteral>(BoolLiteral { TRY(parse<Word>(in, "true")), true });
    } else if (in.peek("false")) {
        return make_shared<BoolLiteral>(BoolLiteral { TRY(parse<Word>(in, "false")), false });
    } else {
        return in.expected("boolean literal");
    }
}
