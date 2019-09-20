#pragma once

#include <string>
#include <memory>
#include <variant>
#include <iostream>

#include <span.hpp>

#define PARSER_TRY(result)                      \
    ({                                          \
        auto res = result;                      \
        if (!res.has_result()) {                \
            return res;                         \
        }                                       \
        std::get<0>(res.res);                   \
    })



namespace ejdi::parser::result {
    class ParserError {
    public:
        span::Span span;
        std::string expected;
        std::string got;

        std::optional<std::unique_ptr<ParserError>> cause;

        ParserError(span::Span span, std::string expected, std::string got)
            : span(span)
            , expected(expected)
            , got(got) {}

        inline void caused_by(std::unique_ptr<ParserError> cause) {
            this->cause = std::move(cause);
        }
    };


    class UnexpectedEoi : public ParserError {
    public:
        UnexpectedEoi(std::string expected)
            : ParserError(span::Span::empty(), expected, "end of input") {}
    };


    template< typename T >
    struct ParserResult {
        std::variant<std::shared_ptr<T>, std::unique_ptr<ParserError>> res;

        ParserResult(std::shared_ptr<T> val) : res(std::move(val)) {}
        ParserResult(std::unique_ptr<ParserError> err) : res(std::move(err)) {}
        ParserResult(std::unique_ptr<UnexpectedEoi> err)
            : res(std::unique_ptr<ParserError>(err.release())) {}

        inline std::optional<std::shared_ptr<T>> opt() {
            if (has_result()) {
                return std::move(std::get<0>(res));
            } else {
                return std::nullopt;
            }
        }

        inline bool has_result() const {
            return std::holds_alternative<std::shared_ptr<T>>(res);
        }

        inline std::shared_ptr<T> get() {
            return std::get<0>(res);
        }

        inline ParserError& error() {
            return *std::get<1>(res);
        }

        template< typename U, typename F >
        ParserResult<U> map(F func) {
            if (has_result()) {
                return ParserResult<U>(func(std::move(std::get<0>(res))));
            } else {
                return ParserResult<U>(std::move(std::get<1>(res)));
            }
        }

        template< typename U >
        operator ParserResult<U>() {
            if (has_result()) {
                dynamic_cast<U&>(*std::get<0>(res));
                return std::dynamic_pointer_cast<U>(std::get<0>(res));
            } else {
                return ParserResult<U>(std::move(std::get<1>(res)));
            }
        }
    };
}
