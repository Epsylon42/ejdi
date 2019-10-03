#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <variant>

#include <span.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>

namespace ejdi::ast {
    template< typename T >
    using Rc = std::shared_ptr<T>;

    struct Assignment;
    struct ExprStmt;

    struct Variable;
    struct Block;
    struct ParenExpr;
    struct BinaryOp;
    struct UnaryOp;
    struct FunctionCall;
    struct FieldAccess;
    struct MethodCall;
    struct WhileLoop;
    struct IfThenElse;
    struct StringLiteral;
    struct NumberLiteral;
    struct BoolLiteral;


    using Stmt = std::variant<Rc<Assignment>, Rc<ExprStmt>>;
    using Expr = std::variant<
        Rc<Variable>,
        Rc<Block>,
        // Rc<ParenExpr>,
        Rc<BinaryOp>,
        Rc<UnaryOp>,
        Rc<FunctionCall>,
        Rc<FieldAccess>,
        Rc<MethodCall>,
        Rc<WhileLoop>,
        Rc<IfThenElse>,
        Rc<StringLiteral>,
        Rc<NumberLiteral>,
        Rc<BoolLiteral>
    >;


    struct Assignment {
        std::optional<lexer::Word> let;
        std::optional<Expr> base;
        lexer::Word field;
        lexer::Punct assignment;
        Expr expr;
        lexer::Punct semi;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct ExprStmt {
        Expr expr;
        lexer::Punct semi;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };


    struct Variable {
        lexer::Word variable;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct Block {
        std::optional<lexer::groups::ParenPair> parens;
        std::vector<Stmt> statements;
        std::optional<Expr> ret;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    // struct ParenExpr {
    //     lexer::groups::ParenPair parens;
    //     Expr inner;

    //     std::string debug(std::size_t depth = 0) const;
    // };

    struct BinaryOp {
        lexer::Punct op;
        Expr left;
        Expr right;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct UnaryOp {
        lexer::Punct op;
        Expr expr;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    template< typename T >
    struct List {
        std::optional<lexer::groups::ParenPair> parens;
        std::vector<T> list;

        span::Span span() const;
    };

    struct FunctionCall {
        Expr function;
        Rc<List<Expr>> arguments;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct FieldAccess {
        Expr base;
        lexer::Punct dot;
        lexer::Word field;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct MethodCall {
        Expr base;
        lexer::Punct dot;
        lexer::Word method;
        Rc<List<Expr>> arguments;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct WhileLoop {
        lexer::Word while_;
        Expr condition;
        Rc<Block> block;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct IfThenElse {
        lexer::Word if_;
        Expr condition;
        Rc<Block> then;
        std::optional<std::tuple<lexer::Word, Expr>> else_;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct NumberLiteral {
        lexer::NumberLit literal;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct StringLiteral {
        lexer::StringLit literal;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };

    struct BoolLiteral {
        lexer::Word word;
        bool value;

        std::string debug(std::size_t depth = 0) const;
        span::Span span() const;
    };


    template< typename T, typename U >
    bool ast_is(const U& ast) {
        return std::holds_alternative<Rc<T>>(ast);
    }

    template< typename T, typename U >
    Rc<T> ast_get(U&& ast) {
        return std::get<Rc<T>>(ast);
    }

    template< typename T >
    span::Span ast_span(const T& ast) {
        return std::visit([](const auto& ast){ return ast->span(); }, ast);
    }


    template< typename T >
    std::string ast_debug(const T& ast, std::size_t depth = 0) {
        using lexer::lexem_debug;
        using lexer::groups::lexem_debug;

        return ast.debug(depth);
    }

    template<>
    inline std::string ast_debug<Expr>(const Expr& ast, std::size_t depth) {
        return std::visit([depth](const auto& expr){ return expr->debug(depth); }, ast);
    }

    template<>
    inline std::string ast_debug<Stmt>(const Stmt& ast, std::size_t depth) {
        return std::visit([depth](const auto& stmt){ return stmt->debug(depth); }, ast);
    }
}
