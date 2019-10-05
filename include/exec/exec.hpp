#include <memory>

#include <exec/value.hpp>
#include <exec/context.hpp>
#include <ast.hpp>

namespace ejdi::exec {
    void exec_program(context::Context& ctx, const ast::Program& prog);
    void exec(context::Context& ctx, const ast::Stmt& stmt);
    value::Value eval(context::Context& ctx, const ast::Expr& expr);
}
