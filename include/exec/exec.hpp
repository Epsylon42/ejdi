#include <memory>

#include <exec/value.hpp>
#include <exec/context.hpp>
#include <ast.hpp>

namespace ejdi::exec {
    void exec(const context::Context& ctx, const ast::Stmt& stmt);
    value::Value eval(const context::Context& ctx, const ast::Expr& expr);
}
