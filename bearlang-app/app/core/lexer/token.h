#pragma once

#include <string>

namespace bearlang {

enum class TokenType {
    EndOfFile,
    Newline,
    Indent,
    Dedent,
    Identifier,
    IntegerLiteral,
    DoubleLiteral,
    StringLiteral,
    KeywordInteger,
    KeywordDouble,
    KeywordString,
    KeywordLogic,
    KeywordIf,
    KeywordElse,
    KeywordWhile,
    KeywordFor,
    KeywordInput,
    KeywordOutput,
    KeywordAnd,
    KeywordOr,
    KeywordNot,
    KeywordFrom,
    KeywordTo,
    KeywordTrue,
    KeywordFalse,
    LeftParen,
    RightParen,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Assign,
    Equal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Comma
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::size_t column;
};

std::string tokenTypeToString(TokenType type);

inline bool isTypeKeyword(TokenType type) {
    return type == TokenType::KeywordInteger ||
           type == TokenType::KeywordDouble ||
           type == TokenType::KeywordString ||
           type == TokenType::KeywordLogic;
}

}  // namespace bearlang
