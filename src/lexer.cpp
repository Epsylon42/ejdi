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


bool Lexem::operator==(const string_view& str) const {
    return this->str == str;
}

string Lexem::debug(size_t depth) const {
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


using LPtr = unique_ptr<Lexem>;
using SpanConstructor = function<Span(size_t)>;

optional<LPtr> get_punct(string_view str, SpanConstructor span);
optional<LPtr> get_paren(string_view str, SpanConstructor span);
optional<LPtr> get_word(string_view str, SpanConstructor span);
optional<LPtr> get_num_lit(string_view str, SpanConstructor span);
optional<LPtr> get_str_lit(string_view str, SpanConstructor span);


vector<unique_ptr<Lexem>> actions::split_string(string_view str, string_view filename) {
    vector<unique_ptr<Lexem>> lexems;
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
                offset += (*lexem)->span.length();
                lexems.push_back(move(*lexem));
                goto cont;
            }
        }


        throw logic_error("Unknown byte at position " + to_string(offset) + " '" + str[offset] + "'");
      cont:;
    }

    return lexems;
}

optional<LPtr> get_punct(string_view str, SpanConstructor span) {
    for (auto punct : punctuation) {
        if (str.starts_with(punct)) {
            return make_unique<Lexem>(span(punct.length()), punct);
        }
    }

    return nullopt;
}

optional<LPtr> get_paren(string_view str, SpanConstructor span) {
    for (auto [op, cl] : parens) {
        if (str.starts_with(op)) {
            return make_unique<Paren>(span(op.length()), op, cl, true);
        } else if (str.starts_with(cl)) {
            return make_unique<Paren>(span(cl.length()), op, cl, false);
        }
    }

    return nullopt;
}

optional<LPtr> get_word(string_view str, SpanConstructor span) {
    if (str[0] == '_' || isalpha(str[0])) {
        size_t word_length = 0;
        while (str[word_length] == '_' || isalnum(str[word_length])) {
            word_length += 1;
        }

        return make_unique<Word>(span(word_length), str.substr(0, word_length));
    }

    return nullopt;
}

optional<LPtr> get_num_lit(string_view str, SpanConstructor span) {
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

        return make_unique<NumberLit>(span(length), str.substr(0, length));
    }

    return nullopt;
}

optional<LPtr> get_str_lit(string_view str, SpanConstructor span) {
    if (str.starts_with('"')) {
        string value;

        size_t offset = 1;
        while (!str.substr(offset).empty() && str[offset] != '"') {
            if (str[offset] == '\\') {
                offset += 1;
                if (str.substr(offset).empty()) {
                    throw logic_error("Unexpected end of file inside a string literal");
                }

                for (auto [escape_seq, replacement] : string_escapes) {
                    if (str.substr(offset).starts_with(escape_seq)) {
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

        return make_unique<StringLit>(span(offset), str.substr(0, offset), move(value));
    }

    return nullopt;
}
