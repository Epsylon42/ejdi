#include <cassert>

#include <exec/exec.hpp>

using namespace std;
using namespace ejdi::ast;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;

namespace ejdi::exec {
    void exec(const Context& ctx, const Stmt& stmt) {
        if (ast_is<Assignment>(stmt)) {
            auto assign = ast_get<Assignment>(stmt);

            if (assign->let.has_value() && !assign->base.has_value()) {
                if (ctx.scope->try_get_no_prototype(assign->field.str) == nullptr) {
                    ctx.scope->set_no_prototype(assign->field.str, eval(ctx, assign->expr));
                } else {
                    assert("variable with the same name already exists in this scope" && 0);
                }
            } else if (assign->base.has_value()) {
                auto base = eval(ctx, *assign->base);
                base.as<Object>()->set(assign->field.str, eval(ctx, assign->expr));
            } else {
                auto var = ctx.scope->try_get(assign->field.str);
                if (var == nullptr) {
                    assert("variable does not exist" && 0);
                }

                *var = eval(ctx, assign->expr);
            }
        } else if (ast_is<ExprStmt>(stmt)) {
            eval(ctx, ast_get<ExprStmt>(stmt)->expr);
        } else {
            assert("invalid statement value" && 0);
        }
    }

    struct Evaluator {
        const Context& ctx;


        Value ev(const Variable& var) {
            return ctx.scope->get(var.variable.str);
        }

        Value ev(const Block& block) {
            for (const auto& stmt : block.statements) {
                exec(ctx, stmt);
            }

            if (block.ret.has_value()) {
                return eval(ctx, *block.ret);
            } else {
                return Unit{};
            }
        }

        Value ev(const FunctionCall& funcall) {
            auto function = eval(ctx, funcall.function).as<Function>();
            vector<Value> args;
            args.reserve(funcall.arguments->list.size());
            for (const auto& arg : funcall.arguments->list) {
                args.push_back(eval(ctx, arg));
            }

            return function->call(ctx, move(args));
        }

        Value ev(const FieldAccess& access) {
            auto base = eval(ctx, access.base);
            return get_vtable(ctx, base).get(access.field.str);
        }

        Value ev(const MethodCall& method) {
            auto base = eval(ctx, method.base);
            auto func = get_vtable(ctx, base).get(method.method.str).as<Function>();
            vector<Value> args;
            args.reserve(method.arguments->list.size() + 1);
            args.push_back(move(base));
            for (const auto& arg : method.arguments->list) {
                args.push_back(eval(ctx, arg));
            }

            return func->call(ctx, move(args));
        }

        Value ev(const WhileLoop& loop) {
            while (eval(ctx, loop.condition).as<bool>()) {
                ev(loop.block);
            }

            return Unit{};
        }

        Value ev(const IfThenElse& cond) {
            if (eval(ctx, cond.condition).as<bool>()) {
                return ev(cond.then);
            } else if (cond.else_.has_value()) {
                return eval(ctx, get<1>(*cond.else_));
            } else {
                return Unit{};
            }
        }

        Value ev(const StringLiteral& lit) {
            return lit.literal.value();
        }

        Value ev(const NumberLiteral& lit) {
            return lit.literal.value();
        }

        Value ev(const BoolLiteral& lit) {
            return lit.value;
        }

        template< typename T >
        Value ev(const T&) {
            assert("unimplemented" && 0);
        }

        template< typename T >
        Value operator() (const shared_ptr<T>& expr) {
            return ev(*expr);
        }
    };

    Value eval(const Context& ctx, const Expr& expr) {
        return std::visit(Evaluator{ ctx }, expr);
    }
}
