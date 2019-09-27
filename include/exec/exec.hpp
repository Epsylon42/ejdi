#include <memory>

#include <exec/value.hpp>
#include <exec/context.hpp>
#include <ast.hpp>

namespace ejdi::exec {
    void exec(std::shared_ptr<context::Context> ctx, const ast::Stmt& stmt);
    std::shared_ptr<value::Value> eval(std::shared_ptr<context::Context> ctx, const ast::Expr& expr);
}
