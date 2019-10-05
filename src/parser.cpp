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
        if (parent == nullptr) {
            return Span::empty();
        } else {
            return parent->surrounding.cl.span;
        }
    } else {
        return get_span(*begin);
    }
}

string ParseStream::str() const {
    string ret;

    if (is_empty()) {
        if (parent == nullptr) {
            ret = "";
        } else {
            ret = parent->surrounding.cl.str;
        }
    } else {
        ret = string(get_str(*begin));
    }

    if (ret.empty()) {
        return "[<empty string>]";
    } else {
        return ret;
    }
}


unique_ptr<ParserError> ParseStream::expected(string expected) const {
    return make_unique<ParserError>(span(), move(expected), str());
}


ParserResult<Expr> parser::parse_primary_expr(ParseStream& in) {
    auto stream = in.clone();

    auto num = DO(parse_number_literal(stream));
    if (num.has_result()) {
        in = stream;
        return Expr(move(num.get()));
    }

    auto str = DO(parse_string_literal(stream));
    if (str.has_result()) {
        in = stream;
        return Expr(move(str.get()));
    }

    auto boolean = DO(parse_bool_literal(stream));
    if (boolean.has_result()) {
        in = stream;
        return Expr(move(boolean.get()));
    }

    auto func = DO(parse_function_literal(stream));
    if (func.has_result()) {
        in = stream;
        return Expr(move(func.get()));
    }

    auto array = DO(parse_array_literal(stream));
    if (array.has_result()) {
        in = stream;
        return Expr(move(array.get()));
    }

    auto block = DO(parse_block(stream));
    if (block.has_result()) {
        in = stream;
        return Expr(move(block.get()));
    }

    auto if_ = DO(parse_conditional(stream));
    if (if_.has_result()) {
        in = stream;
        return Expr(move(if_.get()));
    }

    auto while_ = DO(parse_while_loop(stream));
    if (while_.has_result()) {
        in = stream;
        return Expr(move(while_.get()));
    }

    auto for_ = DO(parse_for_loop(stream));
    if (for_.has_result()) {
        in = stream;
        return Expr(move(for_.get()));
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
            auto dot = TRY(parse_str<Punct>(".", stream));
            auto field = TRY_CRITICAL(parse<Word>(stream));

            auto args = DO(parse_list<Expr>(parse_expr, "()", stream));

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
            auto args = DO(parse_list<Expr>(parse_expr, "()", stream));
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

ParserResult<Expr> parser::parse_expr(ParseStream& in) {
    const auto valid_ops = { "==", "!=", "<=", ">=", "<", ">", "+", "-", "*", "/", "%", "~", "&&", "||" };

    auto expr = TRY(parse_unary_expr(in));


    while (any_of(
            valid_ops.begin(),
            valid_ops.end(),
            [&](auto valid_op){ return in.peek(valid_op); }
            )
        ) {
        auto stream = in;

        auto op = TRY(parse<Punct>(stream));
        auto right = TRY_CRITICAL(parse_unary_expr(stream));

        in = stream;
        expr = make_shared<BinaryOp>(BinaryOp { move(op), move(expr), move(right) });
    }

    return expr;
}

ParserResult<Rc<Assignment>> parser::parse_assignment(ParseStream& in) {
    auto stream = in.clone();

    auto let = try_parse<Word>(bind_front(parse_str<Word>, "let"), stream);

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

    auto assignment = TRY(parse_str<Punct>("=", stream));
    auto expr = TRY_CRITICAL(parse_expr(stream));
    auto semi = TRY_CRITICAL(parse_str<Punct>(";", stream));

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

ParserResult<Rc<ExprStmt>> parser::parse_expr_stmt(ParseStream& in) {
    auto stream = in.clone();

    auto expr = TRY(parse_expr(stream));
    auto semi = TRY(parse_str<Punct>(";", stream));

    in = stream;

    return make_shared<ExprStmt>(ExprStmt{ move(expr), semi });
}

ParserResult<Stmt> parser::parse_stmt(ParseStream& in) {
    auto stream = in.clone();
    auto assignment = DO(parse_assignment(stream));
    if (assignment.has_result()) {
        in = stream;
        return Stmt(move(assignment.get()));
    }

    stream = in.clone();

    auto expr_stmt = DO(parse_expr_stmt(stream));
    if (expr_stmt.has_result()) {
        in = stream;
        return Stmt(move(expr_stmt.get()));
    }

    return in.expected("statement");
}

ParserResult<Rc<Block>> parser::parse_block(ParseStream& in) {
    auto stream = in.clone();
    auto group = TRY(parse<Rc<Group>>(stream));
    auto parens = group->surrounding;
    if (parens.str != "{}") {
        return in.expected("block expression");
    }

    vector<Stmt> statements;
    auto group_stream = ParseStream(*group);
    while (true) {
        if (group_stream.is_empty()) {
            break;
        }

        auto group_stream_clone = group_stream.clone();
        auto stmt = DO(parse_stmt(group_stream_clone));
        if (stmt.has_result()) {
            statements.push_back(stmt.get());
            group_stream = group_stream_clone;
        } else {
            auto expr = TRY(parse_expr(group_stream));
            if (!group_stream.is_empty()) {
                auto err = group_stream.expected("; or }");
                err->critical = true;
                return err;
            }

            in = stream;
            return make_shared<Block>(Block{ move(parens), move(statements), move(expr) });
        }
    }

    in = stream;
    return make_shared<Block>(Block{ move(parens), move(statements), nullopt });
}

ParserResult<Rc<WhileLoop>> parser::parse_while_loop(ParseStream& in) {
    auto stream = in.clone();

    auto while_ = TRY(parse_str<Word>("while", stream));
    auto cond = TRY_CRITICAL(parse_expr(stream));
    auto block = TRY_CRITICAL(parse_block(stream));

    in = stream;

    return make_shared<WhileLoop>(
        WhileLoop {
            while_,
            move(cond),
            move(block),
        }
    );
}

ParserResult<Rc<ForLoop>> parser::parse_for_loop(ParseStream& in) {
    auto stream = in.clone();

    auto for_ = TRY(parse_str<Word>("for", stream));
    auto variable = TRY_CRITICAL(parse<Word>(stream));
    auto in_ = TRY_CRITICAL(parse_str<Word>("in", stream));
    auto iterable = TRY_CRITICAL(parse_expr(stream));

    auto body = TRY_CRITICAL(parse_block(stream));

    in = stream;

    return make_shared<ForLoop>(
        ForLoop {
            for_,
            variable,
            in_,
            move(iterable),
            move(body)
        }
    );
}

ParserResult<Rc<IfThenElse>> parser::parse_conditional(ParseStream& in) {
    auto stream = in.clone();

    auto if_ = TRY(parse_str<Word>("if", stream));
    auto cond = TRY_CRITICAL(parse_expr(stream));
    auto then = TRY_CRITICAL(parse_block(stream));

    optional<tuple<Word, Expr>> else_;
    if (stream.peek("else")) {
        auto else_word = TRY(parse_str<Word>("else", stream));
        auto else_expr = TRY_CRITICAL(parse_expr(stream));

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

ParserResult<Rc<NumberLiteral>> parser::parse_number_literal(ParseStream& in) {
    auto lit = TRY(parse<lexer::NumberLit>(in));
    return make_shared<NumberLiteral>(NumberLiteral { lit });
}

ParserResult<Rc<StringLiteral>> parser::parse_string_literal(ParseStream& in) {
    auto lit = TRY(parse<lexer::StringLit>(in));
    return make_shared<StringLiteral>(StringLiteral { lit });
}

ParserResult<Rc<BoolLiteral>> parser::parse_bool_literal(ParseStream& in) {
    if (in.peek("true")) {
        return make_shared<BoolLiteral>(BoolLiteral { TRY(parse_str<Word>("true", in)), true });
    } else if (in.peek("false")) {
        return make_shared<BoolLiteral>(BoolLiteral { TRY(parse_str<Word>("false", in)), false });
    } else {
        return in.expected("boolean literal");
    }
}

ParserResult<Rc<ArrayLiteral>> parser::parse_array_literal(ParseStream& in) {
    auto list = TRY(parse_list<Expr>(parse_expr, "[]", in));

    return make_shared<ArrayLiteral>(ArrayLiteral { move(list) });
}

ParserResult<Rc<FunctionLiteral>> parser::parse_function_literal(ParseStream& in) {
    auto stream = in.clone();

    auto func = TRY(parse_str<Word>("func", stream));
    auto argnames = TRY_CRITICAL(parse_list<Word>(parse<Word>, "()", stream));
    auto body = TRY_CRITICAL(parse_expr(stream));

    in = stream;

    return make_shared<FunctionLiteral>(FunctionLiteral { func, move(argnames), move(body) });
}


ParserResult<Rc<Program>> parser::parse_program(Group& group) {
    auto stream = ParseStream(group.inner);

    vector<Stmt> statements;
    while (!stream.is_empty()) {
        statements.push_back(TRY(parse_stmt(stream)));
    }

    return make_shared<Program>(Program { move(statements) });
}
