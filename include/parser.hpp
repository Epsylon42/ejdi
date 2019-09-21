#pragma once

#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <variant>
#include <iostream>

#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser_result.hpp>

namespace ejdi::parser {
    struct ParseStream {
        std::vector<lexer::groups::LexemTree>::iterator begin;
        std::vector<lexer::groups::LexemTree>::iterator end;

        ParseStream(std::vector<lexer::groups::LexemTree>& lexems)
            : begin(lexems.begin())
            , end(lexems.end()) {}

        ParseStream clone() const;
        ParseStream& operator= (const ParseStream& other) = default;


        bool is_empty() const;
        span::Span span() const;
        std::string str() const;

        std::unique_ptr<result::ParserError> expected(std::string expected) const;

        template< typename T >
        result::ParserResult<T> parse(std::optional<std::string_view> str = std::nullopt) {
            using namespace result;
            using namespace lexer::groups;

            if (begin >= end) {
                return std::make_unique<UnexpectedEoi>(std::string(str.value_or(typeid(T).name())));
            }

            if (lexem_is<T>(*begin)) {
                if (!str.has_value() || get_str(*begin) == *str) {
                    auto ret = lexer_get<T>(*begin);
                    ++begin;
                    return ret;
                } else {
                    return std::make_unique<ParserError>(span(), std::string(*str), std::string(get_str(*begin)));
                }
            } else {
                return std::make_unique<ParserError>(span(), std::string(str.value_or(typeid(T).name())), std::string(get_str(*begin)));
            }
        }
    };


    template< typename T >
    result::ParserResult<T> parse(ParseStream& in) {
        return in.parse<T>();
    }

    template< typename T >
    result::ParserResult<T> parse(ParseStream& in, std::string_view str) {
        return in.parse<T>(str);
    }

    template< typename T, typename... Args >
    std::optional<T> try_parse(ParseStream& in, Args... args) {
        auto stream = in.clone();
        auto res = parse<T>(stream, args...).opt();
        in = stream;
        return res;
    }

    template<>
    result::ParserResult<ast::Expr> parse<ast::Expr>(ParseStream& in);
    template<>
    result::ParserResult<ast::Rc<ast::Assignment>> parse<ast::Rc<ast::Assignment>>(ParseStream& in);
    template<>
    result::ParserResult<ast::Rc<ast::ExprStmt>> parse<ast::Rc<ast::ExprStmt>>(ParseStream& in);
    template<>
    result::ParserResult<ast::Stmt> parse<ast::Stmt>(ParseStream& in);
    template<>
    result::ParserResult<ast::Rc<ast::Block>> parse<ast::Rc<ast::Block>>(ParseStream& in);
}
