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
        std::vector<std::shared_ptr<lexer::Lexem>>::iterator begin;
        std::vector<std::shared_ptr<lexer::Lexem>>::iterator end;

        ParseStream(std::vector<std::shared_ptr<lexer::Lexem>>& lexems)
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
            if (begin >= end) {
                return std::make_unique<result::UnexpectedEoi>(std::string(str.value_or(T::NAME)));
            }

            try {
                dynamic_cast<T&>(**begin);
                auto ret = std::dynamic_pointer_cast<T>(*begin);

                if (!str.has_value() || ret->str == *str) {
                    ++begin;
                    return ret;
                } else {
                    return std::make_unique<result::ParserError>(span(), std::string(*str), (*begin)->str);
                }
            } catch (std::bad_cast& e) {
                return std::make_unique<result::ParserError>(span(), std::string(str.value_or(T::NAME)), (*begin)->str);
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
    std::optional<std::shared_ptr<T>> parse_opt(ParseStream& in, Args... args) {
        auto stream = in.clone();
        auto res = parse<T>(stream, args...).opt();
        in = stream;
        return res;
    }

    template<>
    result::ParserResult<ast::Expr> parse<ast::Expr>(ParseStream& in);
    template<>
    result::ParserResult<ast::Assignment> parse<ast::Assignment>(ParseStream& in);
    template<>
    result::ParserResult<ast::ExprStmt> parse<ast::ExprStmt>(ParseStream& in);
    template<>
    result::ParserResult<ast::Stmt> parse<ast::Stmt>(ParseStream& in);
    template<>
    result::ParserResult<ast::Block> parse<ast::Block>(ParseStream& in);
}
