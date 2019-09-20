#include <parser.hpp>

using namespace std;
using namespace ejdi;
using namespace ejdi::parser;
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


const char* error::ParserError::what() const noexcept {
    return error.c_str();
}


const char* error::UnexpectedEoi::what() const noexcept {
    return "bbbbbbbbb";
}




template<>
shared_ptr<ast::Expr> parser::parse<ast::Expr>(ParseStream& in) {
    try {
        auto stream = in.clone();
        auto word = parse<lexer::Word>(stream);
        in = stream;
        return make_shared<ast::Variable>(*word);
    } catch (error::ParserError& e) {}

    return parse<ast::Block>(in);
}

template<>
shared_ptr<ast::Assignment> parser::parse<ast::Assignment>(ParseStream& in) {
    auto stream = in.clone();

    auto let = parse_opt<lexer::Word>(stream, "let");
    auto variable = parse<lexer::Word>(stream);
    auto assignment = parse<lexer::Punct>(stream, "=");
    auto expr = parse<ast::Expr>(stream);
    auto semi = parse<lexer::Punct>(stream, ";");

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
shared_ptr<ast::ExprStmt> parser::parse<ast::ExprStmt>(ParseStream& in) {
    auto stream = in.clone();

    auto expr = parse<ast::Expr>(stream);
    auto semi = parse<lexer::Punct>(stream, ";");

    in = stream;

    return make_shared<ast::ExprStmt>(
        expr,
        *semi
        );
}

template<>
shared_ptr<ast::Stmt> parser::parse<ast::Stmt>(ParseStream& in) {
    //TODO: improve error handling
    try {
        auto stream = in.clone();
        auto res = parse<ast::Assignment>(stream);
        in = stream;
        return res;
    } catch (error::ParserError& e) {}

    auto stream = in.clone();
    auto res = parse<ast::ExprStmt>(stream);
    in = stream;
    return res;
}

template<>
shared_ptr<ast::Block> parser::parse<ast::Block>(ParseStream& in) {
    auto stream = in.clone();
    auto group = parse<lexer::groups::Group>(stream);
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

        try {
            auto group_stream_clone = group_stream.clone();
            statements.push_back(parse<ast::Stmt>(group_stream_clone));
            group_stream = group_stream_clone;
        } catch(error::ParserError& e) {
            auto expr = parse<ast::Expr>(group_stream);
            if (!group_stream.is_empty()) {
                throw;
            } else {
                in = stream;
                return make_shared<ast::Block>(move(parens), move(statements), move(expr));
            }
        }
    }

    in = stream;
    return make_shared<ast::Block>(move(parens), move(statements), nullopt);
}
