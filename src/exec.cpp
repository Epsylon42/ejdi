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

            if (assign->let.has_value()) {
                if (ctx.scope->try_get_no_prototype(assign->variable.str) == nullptr) {
                    ctx.scope->set_no_prototype(assign->variable.str, eval(ctx, assign->expr));
                } else {
                    assert("variable with the same name already exists in this scope" && 0);
                }
            } else {
                auto var = ctx.scope->try_get(assign->variable.str);
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
            auto function = eval(ctx, funcall.function);
            if (holds_alternative<shared_ptr<Function>>(function.value)) {
                vector<Value> args;
                args.reserve(funcall.arguments->list.size());
                for (const auto& arg : funcall.arguments->list) {
                    args.push_back(eval(ctx, arg));
                }

                return get<shared_ptr<Function>>(function.value)->call(ctx, move(args));
            } else {
                assert("not a function" && 0);
            }
        }

        Value ev(const StringLiteral& lit) {
            return lit.literal.value();
        }

        Value ev(const NumberLiteral& lit) {
            return lit.literal.value();
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
