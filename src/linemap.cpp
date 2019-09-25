#include <cassert>

#include <linemap.hpp>

using namespace std;
using namespace ejdi::span;

namespace ejdi::linemap {
    Linemap::Linemap(string_view source) {
        for (size_t i = 0; i < source.length(); i++) {
            if (source[i] == '\n') {
                line_breaks.push_back(i);
            }
        }
    }

    size_t Linemap::byte_to_line(size_t byte) const {
        for (size_t i = 0; i < line_breaks.size(); i++) {
            if (byte < line_breaks[i]) {
                return i;
            }
        }

        return line_breaks.size();
    }

    pair<Position, Position> Linemap::span_to_pos_pair(Span span) const {
        size_t fst_line = byte_to_line(span.start);
        Position fst = {
            fst_line,
            fst_line == 0 ? span.start : span.start - line_breaks[fst_line-1]
        };

        size_t snd_line = byte_to_line(span.end);
        Position snd = {
            snd_line,
            snd_line == 0 ? span.end : span.end - line_breaks[snd_line-1]
        };

        return make_pair(fst, snd);
    }

    string_view Linemap::slice(string_view source, Span span) const {
        assert("not implemented" && 0);
        auto [ fst, snd ] = span_to_pos_pair(span);

        return source.substr(
            fst.line == 0 ? 0 : line_breaks[fst.line-1],
            snd.line == 0 ? 0 : line_breaks[snd.line-1]
            );
    }
}
