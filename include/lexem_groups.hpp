#pragma once

#include <exception>
#include <cstdint>
#include <variant>

#include <span.hpp>
#include <lexer.hpp>

namespace ejdi::lexer::groups {
    struct ParenPair : LexemBase {
        Paren op;
        Paren cl;

        ParenPair(Paren op, Paren cl)
            : LexemBase(op.span.join(cl.span), op.str + cl.str)
            , op(std::move(op))
            , cl(std::move(cl)) {}
    };

    struct Group;

    using LexemTree = std::variant<std::shared_ptr<Group>, Lexem>;


    span::Span get_span(const LexemTree& tree);
    std::string_view get_str(const LexemTree& tree);
    std::string lexem_debug(const LexemTree& tree, std::size_t depth = 0);

    template< typename T >
    bool lexem_is(const LexemTree& tree) {
        if (std::holds_alternative<Lexem>(tree)) {
            return std::holds_alternative<T>(std::get<Lexem>(tree));
        } else {
            return false;
        }
    }

    template<>
    inline bool lexem_is<std::shared_ptr<Group>>(const LexemTree& tree) {
        return tree.index() == 0;
    }

    inline bool lexem_is_group(const LexemTree& tree) {
        return lexem_is<std::shared_ptr<Group>>(tree);
    }


    template< typename T >
    T lexer_get(const LexemTree& tree) {
        return std::get<T>(std::get<Lexem>(tree));
    }

    template<>
    inline std::shared_ptr<Group> lexer_get<std::shared_ptr<Group>>(const LexemTree& tree) {
        return std::get<std::shared_ptr<Group>>(tree);
    }


    struct Group : LexemBase {
        ParenPair surrounding;
        std::vector<LexemTree> inner;

        Group(std::vector<LexemTree> inner, ParenPair surrounding);

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


    std::shared_ptr<Group> find_groups(std::vector<Lexem> lexems);
}
