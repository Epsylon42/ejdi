#pragma once

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <tuple>
#include <memory>
#include <optional>
#include <variant>

#include <span.hpp>

namespace ejdi::lexer {
    struct LexemBase {
        static constexpr std::string_view NAME = "lexem";

        span::Span span;
        std::string str;

        LexemBase(span::Span span, std::string str)
            : span(span)
            , str(str) {}

        LexemBase(span::Span span, std::string_view str)
            : span(span)
            , str(std::string(str)) {}

        virtual std::string debug(std::size_t depth = 0) const;

        bool operator==(const std::string_view& str) const;
    };


    struct Punct : LexemBase {
        static constexpr std::string_view NAME = "punctuation";

        template< typename T >
        Punct(span::Span span, T str) : LexemBase(span, str) {}
    };


    struct Word : LexemBase {
        static constexpr std::string_view NAME = "word";

        Word(span::Span span, std::string str) : LexemBase(span, str) {}
        Word(span::Span span, std::string_view str) : LexemBase(span, str) {}
    };


    struct Paren : LexemBase {
        static constexpr std::string_view NAME = "paren";

        std::string_view op;
        std::string_view cl;
        bool opening;

        Paren(span::Span span, std::string_view op, std::string_view cl, bool opening)
            : LexemBase(span, std::string(opening ? op : cl))
            , op(op)
            , cl(cl)
            , opening(opening) {}

        Paren flipped() const;
    };


    struct StringLit : LexemBase {
        std::string val_str;

        StringLit(span::Span span, std::string_view str, std::string value)
            : LexemBase(span, std::string(str))
            , val_str(std::move(value)) {}

        std::string value() const;
    };


    struct NumberLit : LexemBase {
        NumberLit(span::Span span, std::string_view str)
            : LexemBase(span, std::string(str)) {}

        float value() const;
    };


    using Lexem = std::variant<Word, Punct, Paren, StringLit, NumberLit>;


    span::Span get_span(const Lexem& lexem);
    std::string_view get_str(const Lexem& lexem);

    template< typename T >
    std::string lexem_debug(const T& lexem, std::size_t depth = 0) {
        return lexem.debug(depth);
    }
    template<>
    std::string lexem_debug<Lexem>(const Lexem& lexem, std::size_t depth);

    template< typename T >
    bool lexem_is(const Lexem& lexem) {
        return std::holds_alternative<T>(lexem);
    }


    namespace actions {
        constexpr std::array<std::string_view, 24> punctuation {
            ",", ".", ";",
                "==", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "~=",
                "=", "<", ">", "+", "-", "*", "/", "%", "~",
                "&&", "||", "!"
        };

        constexpr std::array<std::tuple<std::string_view, std::string_view>, 3> parens {
            std::tuple{ "(", ")" }, std::tuple{ "[", "]" }, std::tuple{ "{", "}" }
        };

        constexpr std::array<std::tuple<std::string_view, std::string_view>, 1> string_escapes {
            std::tuple{ "n", "\n" },
        };


        std::vector<Lexem> split_string(std::string_view str, std::string_view filename);
    }
}
