#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "token.h"

namespace bearlang {

class LexerError : public std::runtime_error {
public:
    explicit LexerError(const std::string& message) : std::runtime_error(message) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();

private:
    void pushToken(TokenType type, const std::string& lexeme = "");
    void handleIndentation(std::size_t spaces, std::size_t line);
    void emitPendingDedents(std::size_t line);
    void skipComment();
    char peekChar(std::size_t offset = 0) const;
    bool isIdentifierStart(char ch) const;
    bool isIdentifierPart(char ch) const;
    void scanIdentifierOrKeyword();
    void scanNumber();
    void scanString();
    void scanOperator();

    std::string source_;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    bool atLineStart_ = true;
    std::vector<std::size_t> indentStack_{0};
    std::vector<Token> tokens_;
};

}  // namespace bearlang
