#pragma once

#include <vector>
#include <memory>
#include <optional>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>

namespace ejdi::ast {
    struct AstNode {
        virtual std::string debug(std::size_t depth = 0) const = 0;

        virtual ~AstNode() = default;
    };

    struct Expr;
    struct Stmt;


    struct Stmt : AstNode {};

    struct Assignment : Stmt {
        std::optional<lexer::Word> let;
        lexer::Word variable;
        lexer::Punct assignment;
        std::shared_ptr<Expr> expr;
        lexer::Punct semi;

        Assignment(
            std::optional<lexer::Word> let,
            lexer::Word variable,
            lexer::Punct assignment,
            std::shared_ptr<Expr> expr,
            lexer::Punct semi
            )
            : let(let)
            , variable(variable)
            , assignment(assignment)
            , expr(expr)
            , semi(semi) {}

        std::string debug(std::size_t depth = 0) const override;
    };

    struct ExprStmt : Stmt {
        std::shared_ptr<Expr> expr;
        lexer::Punct semi;

        ExprStmt(
            std::shared_ptr<Expr> expr,
            lexer::Punct semi
            )
            : expr(expr)
            , semi(semi) {}

        std::string debug(std::size_t depth = 0) const override;
    };


    struct Expr : AstNode {};

    struct Variable : Expr {
        lexer::Word variable;

        Variable(lexer::Word variable) : variable(variable) {}

        std::string debug(std::size_t depth = 0) const override;
    };

    struct Block : Expr {
        std::optional<lexer::groups::ParenPair> parens;
        std::vector<std::shared_ptr<Stmt>> statements;
        std::optional<std::shared_ptr<Expr>> ret;

        Block(
            std::optional<lexer::groups::ParenPair> parens,
            std::vector<std::shared_ptr<Stmt>> statements,
            std::optional<std::shared_ptr<Expr>> ret
            )
            : parens(std::move(parens))
            , statements(std::move(statements))
            , ret(std::move(ret)) {}

        std::string debug(std::size_t depth = 0) const override;
    };

    struct Paren : Expr {
        lexer::groups::ParenPair parens;
        std::shared_ptr<Expr> inner;

        std::string debug(std::size_t depth = 0) const override;
    };
}
