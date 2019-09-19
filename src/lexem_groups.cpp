#include <string>
#include <stack>
#include <numeric>
#include <iterator>
#include <iostream>

#include <span.hpp>
#include <lexem_groups.hpp>

using namespace std;
using namespace ejdi::lexer;
using namespace ejdi::lexer::groups;
using ejdi::span::Span;


Span span_sum(const vector<unique_ptr<Lexem>>& lexems) {
    if (lexems.empty()) {
        return Span::empty();
    }

    if (lexems.size() == 1) {
        return lexems.front()->span;
    }

    return lexems.front()->span.join(lexems.back()->span);
}

string lexems_join(const vector<unique_ptr<Lexem>>& lexems) {
    return accumulate(
        lexems.cbegin(), lexems.cend(), string(),
        [](string&& acc, const auto& elem) {
            if (!acc.empty()) {
                acc += " ";
            }
            acc += elem->str;
            return acc;
        }
        );
}


Group::Group(vector<unique_ptr<Lexem>> inner)
    : Lexem(span_sum(inner), lexems_join(inner))
    , surrounding(nullopt)
    , inner(move(inner)) {}

Group::Group(vector<unique_ptr<Lexem>> inner, std::unique_ptr<ParenPair> surrounding)
    : Lexem(surrounding->span,
            surrounding->op->str + " " + lexems_join(inner) + " " + surrounding->cl->str)
    , surrounding(move(surrounding))
    , inner(move(inner)) {}

string Group::debug(size_t depth) const {
    string ret;
    for (size_t i = 0; i < depth; i++) {
        ret += "  ";
    }

    ret += "group";
    if (surrounding.has_value()) {
        ret += ' ';
        ret += (*surrounding)->str;
    }

    for (const auto& lexem : inner) {
        ret += '\n';
        ret += lexem->debug(depth + 1);
    }

    return ret;
}


template< typename I >
static I find_closing(Span op_span, const Paren& op, I begin, I end) {
    stack<const Paren*> st;
    st.push(&op);

    while (begin < end) {
        auto opt_paren = (*begin)-> template downcast_ref<Paren>();
        if (opt_paren.has_value()) {
            auto paren = *opt_paren;

            if (paren->opening) {
                st.push(paren);
            } else if (st.top()->op == paren->op) {
                //TODO: use more robust comparison

                st.pop();
            } else {
                throw UnbalancedParenthesis(op_span.join(paren->span), paren->span);
            }
        }

        if (st.empty()) {
            return begin;
        }

        ++begin;
    }

    throw UnbalancedParenthesis(op_span, Span::empty());
}


template< typename I >
static unique_ptr<Group> find_groups_range(optional<unique_ptr<ParenPair>> surrounding, I begin, I end) {
    if (begin >= end) {
        return make_unique<Group>(vector<unique_ptr<Lexem>>());
    }

    vector<unique_ptr<Lexem>> inner;

    while (begin < end) {
        auto opt_paren = (*begin)->template downcast_ref<Paren>();

        if (opt_paren.has_value() && (*opt_paren)->opening) {
            auto paren = *opt_paren;

            auto closing = find_closing(paren->span, *paren, next(begin), end);
            auto surround =
                make_unique<ParenPair>(
                    Lexem::downcast_unique<Paren>(move(*begin)).value(),
                    Lexem::downcast_unique<Paren>(move(*closing)).value()
                    );

            inner.push_back(find_groups_range(move(surround), next(begin), closing));
            begin = closing;
        } else if (opt_paren.has_value() && !(*opt_paren)->opening) {
            auto paren = *opt_paren;

            if (surrounding.has_value()) {
                throw UnbalancedParenthesis((*surrounding)->op->span.join(paren->span), paren->span);
            } else {
                throw UnbalancedParenthesis(paren->span, paren->span);
            }
        } else {
            inner.push_back(move(*begin));
        }

        ++begin;
    }

    if (surrounding.has_value()) {
        return make_unique<Group>(move(inner), move(*surrounding));
    } else {
        return make_unique<Group>(move(inner));
    }
}

unique_ptr<Group> groups::find_groups(std::vector<std::unique_ptr<Lexem>> lexems) {
    return find_groups_range(nullopt, lexems.begin(), lexems.end());
}
