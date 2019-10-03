#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <span.hpp>

namespace ejdi::exec::error {
    struct RuntimeError {
        std::string root_error;
        span::Span root_span = span::Span::empty();

        std::vector<std::tuple<std::string, span::Span>> stack_trace;

        inline void set_span_once(span::Span span) {
            if (root_span.is_empty) {
                root_span = span;
            }
        }
    };
}
