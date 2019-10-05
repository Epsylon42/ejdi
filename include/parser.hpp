#pragma once

#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <variant>
#include <iostream>
#include <functional>

#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser_result.hpp>

namespace ejdi::parser {
    struct ParseStream {
        std::vector<lexer::groups::LexemTree>::iterator begin;
        std::vector<lexer::groups::LexemTree>::iterator end;

    private:
        lexer::groups::Group* parent = nullptr;
    public:

        ParseStream(std::vector<lexer::groups::LexemTree>& lexems)
            : begin(lexems.begin())
            , end(lexems.end()) {}

        ParseStream(lexer::groups::Group& parent)
            : ParseStream(parent.inner)
        {
            this->parent = &parent;
        }

        ParseStream clone() const;
        ParseStream& operator= (const ParseStream& other) = default;


        bool peek(std::string_view str) const;
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
    using Parser = std::function<result::ParserResult<T>(ParseStream&)>;


    template< typename T >
    result::ParserResult<T> parse(ParseStream& in) {
        return in.parse<T>();
    }

    template< typename T >
    result::ParserResult<T> parse_str(std::string_view str, ParseStream& in) {
        return in.parse<T>(str);
    }

    template< typename T >
    std::optional<T> try_parse(Parser<T> parser, ParseStream& in) {
        auto stream = in.clone();
        auto res = parser(stream);
        if (res.has_result()) {
            in = stream;
            return res.opt();
        } else {
            return std::nullopt;
        }
    }

#define PR(X) result::ParserResult<ast::Rc<ast::X>>
    result::ParserResult<ast::Expr> parse_expr(ParseStream& in);

    result::ParserResult<ast::Expr> parse_primary_expr(ParseStream& in);
    result::ParserResult<ast::Expr> parse_unary_expr(ParseStream& in);
    result::ParserResult<ast::Expr> parse_access_expr(ParseStream& in);

    PR(Assignment) parse_assignment(ParseStream& in);
    PR(ExprStmt) parse_expr_stmt(ParseStream& in);
    result::ParserResult<ast::Stmt> parse_stmt(ParseStream& in);

    PR(Block) parse_block(ParseStream& in);
    PR(WhileLoop) parse_while_loop(ParseStream& in);
    PR(IfThenElse) parse_conditional(ParseStream& in);
    PR(NumberLiteral) parse_number_literal(ParseStream& in);
    PR(StringLiteral) parse_string_literal(ParseStream& in);
    PR(BoolLiteral) parse_bool_literal(ParseStream& in);
    PR(ArrayLiteral) parse_array_literal(ParseStream& in);
    PR(FunctionLiteral) parse_function_literal(ParseStream& in);

    template< typename T >
    PR(List<T>) parse_list(
        Parser<T> parse_elem,
        std::optional<std::string_view> parens,
        ParseStream& in)
    {
        using namespace std;
        using namespace ejdi::ast;
        using namespace ejdi::lexer;
        using namespace ejdi::lexer::groups;
        using namespace ejdi::parser::result;

        auto stream = in;
        auto group = TRY(parse<Rc<Group>>(stream));

        if (parens.has_value()) {
            if (group->surrounding.str != *parens) {
                string ret = "group surrounded by ";
                ret += *parens;
                return in.expected(move(ret));
            }
        }

        auto group_stream = ParseStream(group->inner);
        vector<T> items;

        bool mandatory_element = false;
        while (!group_stream.is_empty() || mandatory_element) {
            items.push_back(TRY(parse_elem(group_stream)));
            mandatory_element = false;

            if (group_stream.peek(",")) {
                TRY(parse_str<Punct>(",", group_stream));
                mandatory_element = true;
            }
        }

        in = stream;
        return make_shared<List<T>>(List<T>{ group->surrounding, move(items) });
    }

#undef PR

    result::ParserResult<ast::Rc<ast::Program>> parse_program(lexer::groups::Group& group);
}
