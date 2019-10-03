#include <utility>
#include <cmath>

#include <span.hpp>

using namespace std;
using ejdi::span::Span;

const char* ejdi::span::SpanError::what() const noexcept {
    return "Cannot join spans from different files";
}

Span Span::join(Span other) const {
    if (file != other.file) {
        throw SpanError();
    }
    if (is_empty) {
        return other;
    }
    if (other.is_empty) {
        return *this;
    }

    return Span(
        std::move(other.file),
        std::min(start, other.start),
        std::max(end, other.end)
        );
}

string_view Span::get_substring(string_view source) const {
    return source.substr(start, end - start);
}

std::size_t Span::length() const {
    return end - start;
}

Span Span::empty() {
    auto ret = Span("", 0, 0);
    ret.is_empty = true;
    return ret;
}

bool Span::operator==(const Span& other) const noexcept {
    return !is_empty
        && file == other.file
        && start == other.start
        && end == other.end;
}
