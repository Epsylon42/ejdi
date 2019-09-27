#include <memory>

#include <exec/value.hpp>
#include <ast.hpp>

namespace ejdi::exec {
    void exec(const std::shared_ptr<value::Object>& ctx, const ast::Stmt& stmt);
    value::Value eval(const std::shared_ptr<value::Object>& ctx, const ast::Expr& expr);
}
