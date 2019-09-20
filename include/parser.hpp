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

namespace ejdi::parser {
    namespace error {
        class ParserError : std::exception {
            span::Span span;
            std::string expected;
            std::string got;

            std::string error;

        public:
            ParserError(span::Span span, std::string expected, std::string got)
                : span(span)
                , expected(expected)
                , got(got)
            {
                error = "expected " + expected + " got " + got + " at " + std::to_string(span.start);
            }

            const char* what() const noexcept override;
        };


        class UnexpectedEoi : public ParserError {
        public:
            UnexpectedEoi(std::string expected)
                : ParserError(span::Span::empty(), expected, "end of input") {}

            const char* what() const noexcept override;
        };
    }

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


        template< typename T >
        std::shared_ptr<T> parse(std::optional<std::string_view> str = std::nullopt) {
            if (begin >= end) {
                throw error::UnexpectedEoi(std::string(str.value_or(T::NAME)));
            }

            try {
                dynamic_cast<T&>(**begin);
                auto ret = std::dynamic_pointer_cast<T>(*begin);

                if (!str.has_value() || ret->str == *str) {
                    ++begin;
                    return ret;
                } else {
                    throw error::ParserError((*begin)->span, std::string(*str), (*begin)->str);
                }
            } catch (std::bad_cast& e) {
                throw error::ParserError((*begin)->span, std::string(str.value_or(T::NAME)), (*begin)->str);
            }
        }
    };


    template< typename T >
    std::shared_ptr<T> parse(ParseStream& in) {
        return in.parse<T>();
    }

    template< typename T >
    std::shared_ptr<T> parse(ParseStream& in, std::string_view str) {
        return in.parse<T>(str);
    }

    template< typename T, typename... Args >
    std::optional<std::shared_ptr<T>> parse_opt(ParseStream& in, Args... args) {
        auto stream = in.clone();
        try {
            auto res = parse<T>(stream, args...);
            in = stream;
            return res;
        } catch (error::ParserError& e) {}

        return std::nullopt;
    }

    template<>
    std::shared_ptr<ast::Expr> parse<ast::Expr>(ParseStream& in);
    template<>
    std::shared_ptr<ast::Assignment> parse<ast::Assignment>(ParseStream& in);
    template<>
    std::shared_ptr<ast::ExprStmt> parse<ast::ExprStmt>(ParseStream& in);
    template<>
    std::shared_ptr<ast::Stmt> parse<ast::Stmt>(ParseStream& in);
    template<>
    std::shared_ptr<ast::Block> parse<ast::Block>(ParseStream& in);
}
