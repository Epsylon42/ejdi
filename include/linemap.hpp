#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <utility>

#include <span.hpp>

namespace ejdi::linemap {
    struct Position {
        std::size_t line;
        std::size_t column;
    };

    struct Linemap {
        std::vector<std::size_t> line_breaks;

        Linemap(std::string_view source);

        std::size_t byte_to_line(std::size_t byte) const;
        std::pair<Position, Position> span_to_pos_pair(span::Span span) const;
        std::string_view slice(std::string_view source, span::Span span) const;
    };
}
