#pragma once

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <tuple>
#include <memory>

#include <span.hpp>

namespace ejdi::lexer {
    struct Lexem {
        span::Span span;
        std::string str;

        Lexem(span::Span span, std::string str)
            : span(span)
            , str(str) {}

        Lexem(span::Span span, std::string_view str)
            : span(span)
            , str(std::string(str)) {}

        bool operator==(const std::string_view& str) const;
    };


    struct Word : Lexem {
        Word(span::Span span, std::string str) : Lexem(span, str) {}
        Word(span::Span span, std::string_view str) : Lexem(span, str) {}
    };


    struct Paren : Lexem {
        std::string_view op;
        std::string_view cl;
        bool opening;

        Paren(span::Span span, std::string_view op, std::string_view cl, bool opening)
            : Lexem(span, std::string(opening ? op : cl))
            , op(op)
            , cl(cl)
            , opening(opening) {}

        Paren flipped() const;
    };


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


        std::vector<std::unique_ptr<Lexem>> split_string(std::string_view str, std::string_view filename);
    }
}
