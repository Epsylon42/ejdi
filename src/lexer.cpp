#include <cctype>
#include <string>
#include <iostream>
#include <optional>
#include <functional>

#include <span.hpp>
#include <lexer.hpp>

using namespace std;
using ejdi::span::Span;
using namespace ejdi::lexer;
using namespace ejdi::lexer::actions;


bool LexemBase::operator==(const string_view& str) const {
    return this->str == str;
}

string LexemBase::debug(size_t depth) const {
    string ret;
    for (size_t i = 0; i < depth; i++) {
        ret += "  ";
    }
    ret += str;
    return ret;
}


Paren Paren::flipped() const {
    return Paren(Span::empty(), op, cl, !opening);
}


string StringLit::value() const {
    return val_str;
}


float NumberLit::value() const {
    return stof(str);
}


Span ejdi::lexer::get_span(const Lexem& lexem) {
    return visit([](const auto& lexem) { return lexem.span; }, lexem);
}

string_view ejdi::lexer::get_str(const Lexem& lexem) {
    return *visit([](const auto& lexem) { return &lexem.str; }, lexem);
}

template<>
string ejdi::lexer::lexem_debug<Lexem>(const Lexem& lexem, size_t depth) {
    return visit([depth](const auto& lexem) { return lexem.debug(depth); }, lexem);
}


static bool starts_with(string_view str, string_view pat) {
    return str.length() >= pat.length()
        && str.substr(0, pat.length()) == pat;
}

static bool starts_with(string_view str, char c) {
    return !str.empty() && str[0] == c;
}


using SpanConstructor = function<Span(size_t)>;

optional<Lexem> get_punct(string_view str, SpanConstructor span);
optional<Lexem> get_paren(string_view str, SpanConstructor span);
optional<Lexem> get_word(string_view str, SpanConstructor span);
optional<Lexem> get_num_lit(string_view str, SpanConstructor span);
optional<Lexem> get_str_lit(string_view str, SpanConstructor span);


vector<Lexem> actions::split_string(string_view str, string_view filename) {
    vector<Lexem> lexems;
    size_t offset = 0;

    const auto functions = { get_punct, get_paren, get_word, get_num_lit, get_str_lit };

    while(true) {
        while (!str.substr(offset).empty() && isspace(str[offset])) {
            offset += 1;
        }
        if (str.substr(offset).empty()) {
            break;
        }

        auto span_constructor = [offset, filename](size_t length) {
            return Span(string(filename), offset, offset + length);
        };

        for (auto& func : functions) {
            auto lexem = func(str.substr(offset), span_constructor);
            if (lexem.has_value()) {
                offset += get_span(*lexem).length();
                lexems.push_back(move(*lexem));
                goto cont;
            }
        }


        throw logic_error("Unknown byte at position " + to_string(offset) + " '" + str[offset] + "'");
      cont:;
    }

    return lexems;
}

optional<Lexem> get_punct(string_view str, SpanConstructor span) {
    for (auto punct : punctuation) {
        if (starts_with(str, punct)) {
            return Punct(span(punct.length()), punct);
        }
    }

    return nullopt;
}

optional<Lexem> get_paren(string_view str, SpanConstructor span) {
    for (auto [op, cl] : parens) {
        if (starts_with(str, op)) {
            return Paren(span(op.length()), op, cl, true);
        } else if (starts_with(str, cl)) {
            return Paren(span(cl.length()), op, cl, false);
        }
    }

    return nullopt;
}

optional<Lexem> get_word(string_view str, SpanConstructor span) {
    if (str[0] == '_' || isalpha(str[0])) {
        size_t word_length = 0;
        while (str[word_length] == '_' || isalnum(str[word_length])) {
            word_length += 1;
        }

        return Word(span(word_length), str.substr(0, word_length));
    }

    return nullopt;
}

optional<Lexem> get_num_lit(string_view str, SpanConstructor span) {
    if (isdigit(str[0])) {
        size_t length = 0;
        bool dot = false;
        while (isdigit(str[length]) || str[length] == '.') {
            if (str[length] == '.') {
                if (!dot) {
                    dot = true;
                } else {
                    break;
                }
            }

            length += 1;
        }

        return NumberLit(span(length), str.substr(0, length));
    }

    return nullopt;
}

optional<Lexem> get_str_lit(string_view str, SpanConstructor span) {
    if (starts_with(str, '"')) {
        string value;

        size_t offset = 1;
        while (!str.substr(offset).empty() && str[offset] != '"') {
            if (str[offset] == '\\') {
                offset += 1;
                if (str.substr(offset).empty()) {
                    throw logic_error("Unexpected end of file inside a string literal");
                }

                for (auto [escape_seq, replacement] : string_escapes) {
                    if (starts_with(str.substr(offset), escape_seq)) {
                        offset += escape_seq.length();
                        value += replacement;
                        goto cont;
                    }
                }
            }

            value += str[offset];
            offset += 1;

          cont:;
        }

        if (str.substr(offset).empty()) {
            throw logic_error("Unexpected end of file inside a string literal");
        } else {
            offset += 1;
        }

        return StringLit(span(offset), str.substr(0, offset), move(value));
    }

    return nullopt;
}
