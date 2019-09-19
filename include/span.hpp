#pragma once

#include <string>
#include <exception>
#include <cstdint>
#include <string_view>

namespace ejdi::span {
    class SpanError : public std::exception {
        const char* what() const noexcept override;
    };

    struct Span {
        std::string file;
        std::size_t start;
        std::size_t end;
        bool is_empty;

        Span(std::string file, std::size_t start, std::size_t end)
            : file(file)
            , start(start)
            , end(end)
            , is_empty(false) {}

        static Span empty();


        Span join(Span other) const;
        std::string_view get_substring(std::string_view source) const;
        std::size_t length() const;


        bool operator==(const Span& other) const noexcept;
    };
}
