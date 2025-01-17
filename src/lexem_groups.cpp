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


static string lexems_join(const vector<LexemTree>& lexems) {
    return accumulate(
        lexems.cbegin(), lexems.cend(), string(),
        [](string acc, const auto& elem) {
            if (!acc.empty()) {
                acc += " ";
            }
            acc += get_str(elem);
            return acc;
        }
        );
}


Span groups::get_span(const LexemTree& tree) {
    if (lexem_is_group(tree)) {
        return get<0>(tree)->span;
    } else {
        return get_span(get<1>(tree));
    }
}

string_view groups::get_str(const LexemTree& tree) {
    if (lexem_is_group(tree)) {
        return get<0>(tree)->str;
    } else {
        return get_str(get<1>(tree));
    }
}

string groups::lexem_debug(const LexemTree& tree, size_t depth) {
    if (lexem_is_group(tree)) {
        return get<0>(tree)->debug(depth);
    } else {
        return lexem_debug(get<1>(tree), depth);
    }
}


Group::Group(vector<LexemTree> inner, ParenPair surrounding)
    : LexemBase(surrounding.span,
            surrounding.op.str + " " + lexems_join(inner) + " " + surrounding.cl.str)
    , surrounding(move(surrounding))
    , inner(move(inner)) {}

string Group::debug(size_t depth) const {
    string ret;
    for (size_t i = 0; i < depth; i++) {
        ret += "  ";
    }

    ret += "group ";
    ret += surrounding.str;

    for (const auto& lexem : inner) {
        ret += '\n';
        ret += lexem_debug(lexem, depth + 1);
    }

    return ret;
}


template< typename I >
static I find_closing(Span op_span, const Paren& op, I begin, I end) {
    stack<const Paren*> st;
    st.push(&op);

    while (begin < end) {
        if (lexem_is<Paren>(*begin)) {
            const auto& paren = get<Paren>(*begin);

            if (paren.opening) {
                st.push(&paren);
            } else if (st.top()->op == paren.op) {
                //TODO: use more robust comparison
                st.pop();
            } else {
                throw UnbalancedParenthesis(op_span.join(paren.span), paren.span);
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
static shared_ptr<Group> find_groups_range(ParenPair surrounding, I begin, I end) {
    if (begin >= end) {
        return make_shared<Group>(vector<LexemTree>(), move(surrounding));
    }

    vector<LexemTree> inner;

    while (begin < end) {
        if (lexem_is<Paren>(*begin)) {
            const auto& paren = get<Paren>(*begin);

            if (paren.opening) {
                auto closing = find_closing(paren.span, paren, next(begin), end);
                auto surround = ParenPair(
                    move(get<Paren>(*begin)),
                    move(get<Paren>(*closing))
                    );

                inner.push_back(find_groups_range(move(surround), next(begin), closing));
                begin = closing;
            } else {
                throw UnbalancedParenthesis(surrounding.op.span.join(paren.span), paren.span);
            }
        } else {
            inner.push_back(move(*begin));
        }

        ++begin;
    }

    return make_shared<Group>(move(inner), move(surrounding));
}

shared_ptr<Group> groups::find_groups(std::vector<Lexem> lexems) {
    auto pair = ParenPair(
        Paren(Span::empty(), "", "", true),
        Paren(Span::empty(), "", "", false));

    return find_groups_range(move(pair), lexems.begin(), lexems.end());
}
