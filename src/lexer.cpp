#include <cctype>
#include <iostream>

#include <span.hpp>
#include <lexer.hpp>

using namespace std;
using ejdi::span::Span;
using namespace ejdi::lexer;


bool Lexem::operator==(const string_view& str) const {
    return this->str == str;
}


Paren Paren::flipped() const {
    return Paren(Span::empty(), op, cl, !opening);
}


vector<unique_ptr<Lexem>> actions::split_string(string_view str, string_view filename) {
    vector<unique_ptr<Lexem>> lexems;
    size_t offset = 0;

    while(true) {
        while (isspace(str[0]) && !str.empty()) {
            str = str.substr(1);
            offset += 1;
        }
        if (str.empty()) {
            break;
        }

        for (auto punct : punctuation) {
            if (str.starts_with(punct)) {
                lexems.emplace_back(
                    new Lexem(
                        Span(string(filename), offset, offset + punct.length()),
                        punct
                        )
                    );
                str = str.substr(punct.length());
                offset += punct.length();
                goto cont;
            }
        }

        for (auto [op, cl] : parens) {
            if (str.starts_with(op)) {
                lexems.emplace_back(
                    new Paren(
                        Span(string(filename), offset, offset + op.length()),
                        op, cl,
                        true
                        )
                    );
                str = str.substr(op.length());
                offset += op.length();
                goto cont;
            } else if (str.starts_with(cl)) {
                lexems.emplace_back(
                    new Paren(
                        Span(string(filename), offset, offset + op.length()),
                        op, cl,
                        true
                        )
                    );
                str = str.substr(cl.length());
                offset += cl.length();
                goto cont;
            }
        }

        if (str[0] == '_' || isalpha(str[0])) {
            size_t word_length = 0;
            while (str[word_length] == '_' || isalnum(str[word_length])) {
                word_length += 1;
            }

            lexems.emplace_back(
                new Word(
                    Span(string(filename), offset, offset + word_length),
                    str.substr(0, word_length)
                    )
                );
            str = str.substr(word_length);
            offset += word_length;
            goto cont;
        }

        throw logic_error("Unknown byte at position " + to_string(offset) + " '" + str[0] + "'");
      cont:;
    }

    return lexems;
}
