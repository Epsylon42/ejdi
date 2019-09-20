#include <parser.hpp>

using namespace std;
using namespace ejdi;
using namespace ejdi::parser;
using namespace ejdi::parser::result;
using ejdi::span::Span;

ParseStream ParseStream::clone() const {
    return *this;
}

bool ParseStream::is_empty() const {
    return begin >= end;
}

Span ParseStream::span() const {
    if (is_empty()) {
        return Span::empty();
    } else {
        return (*begin)->span;
    }
}

string ParseStream::str() const {
    if (is_empty()) {
        return "";
    } else {
        return (*begin)->str;
    }
}

unique_ptr<ParserError> ParseStream::expected(string expected) const {
    return make_unique<ParserError>(span(), move(expected), str());
}


template<>
ParserResult<ast::Expr> parser::parse<ast::Expr>(ParseStream& in) {
    auto stream = in.clone();
    auto word = parse<lexer::Word>(stream);
    if (word.has_result()) {
        in = stream;
        return word.map<ast::Variable>(
            [](auto word){ return make_shared<ast::Variable>(*word); }
            );
    }

    auto block = PARSER_TRY(parse<ast::Block>(stream));
    in = stream;
    return ParserResult(block);
}

template<>
ParserResult<ast::Assignment> parser::parse<ast::Assignment>(ParseStream& in) {
    auto stream = in.clone();

    auto let = parse_opt<lexer::Word>(stream, "let");
    auto variable = PARSER_TRY(parse<lexer::Word>(stream));
    auto assignment = PARSER_TRY(parse<lexer::Punct>(stream, "="));
    auto expr = PARSER_TRY(parse<ast::Expr>(stream));
    auto semi = PARSER_TRY(parse<lexer::Punct>(stream, ";"));

    in = stream;

    auto let_ = let.has_value() ? optional(**let) : nullopt;

    return make_shared<ast::Assignment>(
        let_,
        *variable,
        *assignment,
        expr,
        *semi
        );
}

template<>
ParserResult<ast::ExprStmt> parser::parse<ast::ExprStmt>(ParseStream& in) {
    auto stream = in.clone();

    auto expr = PARSER_TRY(parse<ast::Expr>(stream));
    auto semi = PARSER_TRY(parse<lexer::Punct>(stream, ";"));

    in = stream;

    return make_shared<ast::ExprStmt>(
        expr,
        *semi
        );
}

template<>
ParserResult<ast::Stmt> parser::parse<ast::Stmt>(ParseStream& in) {
    //TODO: improve error handling
    auto stream = in.clone();
    auto assignment = parse<ast::Assignment>(stream);
    if (assignment.has_result()) {
        in = stream;
        return assignment;
    }

    stream = in.clone();

    auto res = parse<ast::ExprStmt>(stream);

    in = stream;
    return res;
}

template<>
ParserResult<ast::Block> parser::parse<ast::Block>(ParseStream& in) {
    auto stream = in.clone();
    auto group = PARSER_TRY(parse<lexer::groups::Group>(stream));
    optional<lexer::groups::ParenPair> parens;
    if (group->surrounding.has_value()) {
        parens = move(**group->surrounding);
    }

    vector<shared_ptr<ast::Stmt>> statements;
    ParseStream group_stream(group->inner);
    while (true) {
        if (group_stream.is_empty()) {
            break;
        }

        auto group_stream_clone = group_stream.clone();
        auto stmt = parse<ast::Stmt>(group_stream_clone);
        if (stmt.has_result()) {
            statements.push_back(*stmt.opt());
            group_stream = group_stream_clone;
        } else {
            auto expr = PARSER_TRY(parse<ast::Expr>(group_stream));
            if (!group_stream.is_empty()) {
                return group_stream.expected("; or }");
            }

            in = stream;
            return make_shared<ast::Block>(move(parens), move(statements), move(expr));
        }
    }

    in = stream;
    return make_shared<ast::Block>(move(parens), move(statements), nullopt);
}
