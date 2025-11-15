#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "core/lexer/token.h"
#include "ast.h"

namespace bearlang {

class ParserError : public std::runtime_error {
public:
    explicit ParserError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    Program parseProgram();

private:
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    const Token& advance();
    const Token& consume(TokenType type, const std::string& message);
    bool match(TokenType type);
    void skipNewlines();
    void expectNewline(const std::string& context);

    ValueType parseTypeKeyword(const std::string& context);

    StmtPtr parseStatement();
    StmtPtr parseVarDecl();
    StmtPtr parseAssignment();
    StmtPtr parseInput();
    StmtPtr parseOutput();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();

    std::vector<StmtPtr> parseIndentedBlock(const std::string& context);

    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parsePower();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();

    ExprPtr parseParenthesizedCondition(const std::string& context);

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

}  // namespace bearlang
