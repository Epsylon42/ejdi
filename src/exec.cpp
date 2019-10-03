#include <cassert>
#include <cstring>

#include <exec/exec.hpp>
#include <exec/error.hpp>

using namespace std;
using namespace ejdi::ast;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;
using namespace ejdi::exec::error;

namespace ejdi::exec {
    void exec(Context& ctx, const Stmt& stmt) {
        try {
            if (ast_is<Assignment>(stmt)) {
                auto assign = ast_get<Assignment>(stmt);

                if (assign->let.has_value() && !assign->base.has_value()) {
                    if (ctx.scope->try_get_no_prototype(assign->field.str) == nullptr) {
                        ctx.scope->set_no_prototype(assign->field.str, eval(ctx, assign->expr));
                    } else {
                        string msg = "variable with name '";
                        msg += assign->field.str;
                        msg += "' already exists in this scope";
                        throw ctx.error(move(msg), assign->field.span);
                    }
                } else if (assign->base.has_value()) {
                    auto base = eval(ctx, *assign->base);
                    base.as<Object>()->set(assign->field.str, eval(ctx, assign->expr));
                } else {
                    auto var = ctx.scope->try_get(assign->field.str);
                    if (var == nullptr) {
                        string msg = "variable '";
                        msg += assign->field.str;
                        msg += "' does not exist";
                        throw ctx.error(move(msg), assign->field.span);
                    }

                    *var = eval(ctx, assign->expr);
                }
            } else if (ast_is<ExprStmt>(stmt)) {
                eval(ctx, ast_get<ExprStmt>(stmt)->expr);
            } else {
                assert("invalid statement value" && 0);
            }
        } catch (RuntimeError& e) {
            e.set_span_once(ast_span(stmt));
            throw;
        }
    }

    struct Comparator {
        Value& left;
        Value& right;

        int cmp(Unit) {
            return true;
        }

        int cmp(float) {
            auto a = left.as<float>();
            auto b = right.as<float>();
            if (a < b) {
                return -1;
            } else if (a > b) {
                return 1;
            } else {
                return 0;
            }
        }

        int cmp(bool) {
            return (int)left.as<bool>() - (int)right.as<bool>();
        }

        int cmp(const shared_ptr<string>&) {
            auto& a = left.as<string>();
            auto& b = right.as<string>();

            if (a->size() != b->size()) {
                return false;
            }

            return strncmp(a->data(), b->data(), a->size());
        }

        int cmp(const shared_ptr<Function>&) {
            return left.as<Function>() == right.as<Function>();
        }

        int cmp(const shared_ptr<Object>&) {
            return left.as<Object>() == right.as<Object>();
        }

        int compare() {
            return visit([this](const auto& x){ return this->cmp(x); }, left.value);
        }
    };

    struct Evaluator {
        Context& ctx;


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

        Value ev(const BinaryOp& op) {
            const auto& str = op.op.str;
            auto left = eval(ctx, op.left);
            auto right = eval(ctx, op.right);

            if (str == "+") {
                return left.as<float>() + right.as<float>();
            } else if (str == "-") {
                return left.as<float>() - right.as<float>();
            } else if (str == "*") {
                return left.as<float>() - right.as<float>();
            } else if (str == "/") {
                return left.as<float>() / right.as<float>();
            } else if (str == "%") {
                return (float)((long)left.as<float>() % (long)right.as<float>());
            } else if (str == "~") {
                auto& ptr = left.as<string>();
                if (ptr.unique()) {
                    *ptr += *right.as<string>();
                    return left;
                } else {
                    auto res = make_shared<string>(*left.as<string>());
                    *res += *right.as<string>();
                    return res;
                }
            } else if (str == "&&") {
                return left.as<bool>() && right.as<bool>();
            } else if (str == "||") {
                return left.as<bool>() || right.as<bool>();
            } else if (str == "==") {
                return Comparator{ left, right }.compare() == 0;
            } else if (str == "!=") {
                return Comparator{ left, right }.compare() != 0;
            } else if (str == "<") {
                return Comparator{ left, right }.compare() < 0;
            } else if (str == ">") {
                return Comparator{ left, right }.compare() > 0;
            } else if (str == "<=") {
                return Comparator{ left, right }.compare() <= 0;
            } else if (str == ">=") {
                return Comparator{ left, right }.compare() >= 0;
            } else {
                assert("invalid binary operator" && 0);
            }
        }

        Value ev(const UnaryOp& op) {
            if (op.op.str == "!") {
                return !eval(ctx, op.expr).as<bool>();
            } else if (op.op.str == "+") {
                return +eval(ctx, op.expr).as<float>();
            } else if (op.op.str == "-") {
                return -eval(ctx, op.expr).as<float>();
            } else {
                assert("invalid unary operator" && 0);
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
                ev(*loop.block);
            }

            return Unit{};
        }

        Value ev(const IfThenElse& cond) {
            if (eval(ctx, cond.condition).as<bool>()) {
                return ev(*cond.then);
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
        Value operator() (const shared_ptr<T>& expr) {
            try {
                return ev(*expr);
            } catch (RuntimeError& e) {
                e.set_span_once(expr->span());
                throw;
            }
        }
    };

    Value eval(Context& ctx, const Expr& expr) {
        return std::visit(Evaluator{ ctx }, expr);
    }
}
