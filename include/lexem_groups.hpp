#pragma once

#include <exception>
#include <cstdint>

#include <lexer.hpp>

namespace ejdi::lexer::groups {
    struct ParenPair : Lexem {
        std::unique_ptr<Paren> op;
        std::unique_ptr<Paren> cl;

        ParenPair(std::unique_ptr<Paren> op, std::unique_ptr<Paren> cl)
            : Lexem(op->span.join(cl->span), op->str + cl->str)
            , op(std::move(op))
            , cl(std::move(cl)) {}
    };


    struct Group : Lexem {
        std::optional<std::unique_ptr<ParenPair>> surrounding;
        std::vector<std::unique_ptr<Lexem>> inner;

        Group(std::vector<std::unique_ptr<Lexem>> inner);
        Group(std::vector<std::unique_ptr<Lexem>> inner,
              std::unique_ptr<ParenPair> surrounding);

        std::string debug(std::size_t depth = 0) const override;
    };


    class UnbalancedParenthesis : public std::logic_error {
    public:
        span::Span full_span;
        span::Span cl_span;

        UnbalancedParenthesis(span::Span full_span, span::Span cl_span)
            : std::logic_error("Unbalanced parenthesis at " + std::to_string(cl_span.start))
            , full_span(full_span)
            , cl_span(cl_span) {}
    };

    std::unique_ptr<Group> find_groups(std::vector<std::unique_ptr<Lexem>> lexems);
}
