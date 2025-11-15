#include "parser.h"

#include <sstream>

namespace bearlang {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Program Parser::parseProgram() {
    Program program;
    skipNewlines();
    while (!isAtEnd()) {
        program.statements.push_back(parseStatement());
        skipNewlines();
    }
    return program;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return false;
    }
    return peek().type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw ParserError(message);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::skipNewlines() {
    while (match(TokenType::Newline)) {
    }
}

void Parser::expectNewline(const std::string& context) {
    if (match(TokenType::Newline)) {
        skipNewlines();
        return;
    }
    if (check(TokenType::Dedent) || check(TokenType::EndOfFile)) {
        return;
    }
    std::ostringstream oss;
    oss << "Ожидается перевод строки после " << context;
    throw ParserError(oss.str());
}

ValueType Parser::parseTypeKeyword(const std::string& context) {
    if (match(TokenType::KeywordInteger)) {
        return ValueType::Integer;
    }
    if (match(TokenType::KeywordDouble)) {
        return ValueType::Double;
    }
    if (match(TokenType::KeywordString)) {
        return ValueType::String;
    }
    if (match(TokenType::KeywordLogic)) {
        return ValueType::Boolean;
    }
    std::ostringstream oss;
    oss << "Ожидается тип для " << context;
    throw ParserError(oss.str());
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::Indent)) {
        throw ParserError("Неожиданный отступ");
    }

    if (isTypeKeyword(peek().type)) {
        return parseVarDecl();
    }

    switch (peek().type) {
        case TokenType::KeywordInput:
            return parseInput();
        case TokenType::KeywordOutput:
            return parseOutput();
        case TokenType::KeywordIf:
            return parseIf();
        case TokenType::KeywordWhile:
            return parseWhile();
        case TokenType::KeywordFor:
            return parseFor();
        case TokenType::Identifier:
            return parseAssignment();
        default: {
            std::ostringstream oss;
            oss << "Неожиданное слово '" << peek().lexeme << "'";
            throw ParserError(oss.str());
        }
    }
}

StmtPtr Parser::parseVarDecl() {
    const Token& typeToken = advance();
    ValueType type;
    switch (typeToken.type) {
        case TokenType::KeywordInteger: type = ValueType::Integer; break;
        case TokenType::KeywordDouble: type = ValueType::Double; break;
        case TokenType::KeywordString: type = ValueType::String; break;
        case TokenType::KeywordLogic: type = ValueType::Boolean; break;
        default: type = ValueType::Unknown; break;
    }

    const Token& name = consume(TokenType::Identifier, "Ожидается имя переменной");
    ExprPtr initializer = nullptr;
    if (match(TokenType::Assign)) {
        initializer = parseExpression();
    }
    auto stmt = makeVarDecl(type, name.lexeme, std::move(initializer));
    expectNewline("объявления переменной");
    return stmt;
}

StmtPtr Parser::parseAssignment() {
    const Token& name = advance();
    consume(TokenType::Assign, "Ожидается '=' в присваивании");
    auto value = parseExpression();
    auto stmt = makeAssign(name.lexeme, std::move(value));
    expectNewline("присваивания");
    return stmt;
}

StmtPtr Parser::parseInput() {
    advance();  // consume keyword
    const Token& name = consume(TokenType::Identifier, "Ожидается переменная для ввода");
    auto stmt = makeInput(name.lexeme);
    expectNewline("оператора ввода");
    return stmt;
}

StmtPtr Parser::parseOutput() {
    advance();
    auto value = parseExpression();
    auto stmt = makeOutput(std::move(value));
    expectNewline("оператора вывода");
    return stmt;
}

StmtPtr Parser::parseIf() {
    advance();
    auto condition = parseParenthesizedCondition("если");
    auto ifBody = parseIndentedBlock("условия 'если'");

    Statement::If ifStmt;
    Statement::IfBranch firstBranch;
    firstBranch.condition = std::move(condition);
    firstBranch.body = std::move(ifBody);
    ifStmt.branches.push_back(std::move(firstBranch));

    while (match(TokenType::KeywordElse)) {
        if (match(TokenType::KeywordIf)) {
            auto elseIfCond = parseParenthesizedCondition("иначе если");
            auto elseIfBody = parseIndentedBlock("условия 'иначе если'");
            Statement::IfBranch branch;
            branch.condition = std::move(elseIfCond);
            branch.body = std::move(elseIfBody);
            ifStmt.branches.push_back(std::move(branch));
        } else {
            ifStmt.elseBranch = parseIndentedBlock("блока 'иначе'");
            ifStmt.hasElse = true;
            break;
        }
    }

    return std::make_unique<Statement>(Statement{Statement::Variant{std::move(ifStmt)}});
}

StmtPtr Parser::parseWhile() {
    advance();
    auto condition = parseParenthesizedCondition("пока");
    auto body = parseIndentedBlock("цикла 'пока'");
    return makeWhile(std::move(condition), std::move(body));
}

StmtPtr Parser::parseFor() {
    advance();
    consume(TokenType::LeftParen, "Ожидается '(' после 'для'");
    auto type = parseTypeKeyword("цикла 'для'");
    const Token& name = consume(TokenType::Identifier, "Ожидается имя счётчика");
    consume(TokenType::KeywordFrom, "Ожидается слово 'от' в цикле");
    auto from = parseExpression();
    consume(TokenType::KeywordTo, "Ожидается слово 'до' в цикле");
    auto to = parseExpression();
    consume(TokenType::RightParen, "Ожидается ')' после заголовка цикла");
    auto body = parseIndentedBlock("цикла 'для'");
    return makeFor(type, name.lexeme, std::move(from), std::move(to), std::move(body));
}

std::vector<StmtPtr> Parser::parseIndentedBlock(const std::string& context) {
    consume(TokenType::Newline, "Ожидается новая строка после " + context);
    consume(TokenType::Indent, "Ожидается отступ после " + context);
    std::vector<StmtPtr> body;
    skipNewlines();
    while (!check(TokenType::Dedent) && !isAtEnd()) {
        body.push_back(parseStatement());
        skipNewlines();
    }
    consume(TokenType::Dedent, "Ожидается завершение блока " + context);
    return body;
}

ExprPtr Parser::parseExpression() { return parseOr(); }

ExprPtr Parser::parseOr() {
    auto expr = parseAnd();
    while (match(TokenType::KeywordOr)) {
        auto right = parseAnd();
        expr = makeBinary("||", std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseAnd() {
    auto expr = parseEquality();
    while (match(TokenType::KeywordAnd)) {
        auto right = parseEquality();
        expr = makeBinary("&&", std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    auto expr = parseComparison();
    while (match(TokenType::Equal)) {
        auto right = parseComparison();
        expr = makeBinary("==", std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseComparison() {
    auto expr = parseTerm();
    while (true) {
        if (match(TokenType::Less)) {
            auto right = parseTerm();
            expr = makeBinary("<", std::move(expr), std::move(right));
        } else if (match(TokenType::LessEqual)) {
            auto right = parseTerm();
            expr = makeBinary("<=", std::move(expr), std::move(right));
        } else if (match(TokenType::Greater)) {
            auto right = parseTerm();
            expr = makeBinary(">", std::move(expr), std::move(right));
        } else if (match(TokenType::GreaterEqual)) {
            auto right = parseTerm();
            expr = makeBinary(">=", std::move(expr), std::move(right));
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parseTerm() {
    auto expr = parseFactor();
    while (true) {
        if (match(TokenType::Plus)) {
            auto right = parseFactor();
            expr = makeBinary("+", std::move(expr), std::move(right));
        } else if (match(TokenType::Minus)) {
            auto right = parseFactor();
            expr = makeBinary("-", std::move(expr), std::move(right));
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parseFactor() {
    auto expr = parsePower();
    while (true) {
        if (match(TokenType::Star)) {
            auto right = parsePower();
            expr = makeBinary("*", std::move(expr), std::move(right));
        } else if (match(TokenType::Slash)) {
            auto right = parsePower();
            expr = makeBinary("/", std::move(expr), std::move(right));
        } else if (match(TokenType::Percent)) {
            auto right = parsePower();
            expr = makeBinary("%", std::move(expr), std::move(right));
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parsePower() {
    auto expr = parseUnary();
    if (match(TokenType::Caret)) {
        auto right = parsePower();
        expr = makeBinary("^", std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::Minus)) {
        auto operand = parseUnary();
        return makeUnary("-", std::move(operand));
    }
    if (match(TokenType::KeywordNot)) {
        auto operand = parseUnary();
        return makeUnary("!", std::move(operand));
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::IntegerLiteral)) {
        return makeLiteral(ValueType::Integer, previous().lexeme);
    }
    if (match(TokenType::DoubleLiteral)) {
        return makeLiteral(ValueType::Double, previous().lexeme);
    }
    if (match(TokenType::StringLiteral)) {
        return makeLiteral(ValueType::String, previous().lexeme);
    }
    if (match(TokenType::KeywordTrue)) {
        return makeLiteral(ValueType::Boolean, "true", true);
    }
    if (match(TokenType::KeywordFalse)) {
        return makeLiteral(ValueType::Boolean, "false", false);
    }
    if (match(TokenType::Identifier)) {
        return makeVariable(previous().lexeme);
    }
    if (match(TokenType::LeftParen)) {
        auto expr = parseExpression();
        consume(TokenType::RightParen, "Ожидается ')' ");
        return expr;
    }

    std::ostringstream oss;
    oss << "Неожиданный токен '" << peek().lexeme << "'";
    throw ParserError(oss.str());
}

ExprPtr Parser::parseParenthesizedCondition(const std::string& context) {
    consume(TokenType::LeftParen, "Ожидается '(' после " + context);
    auto condition = parseExpression();
    consume(TokenType::RightParen, "Ожидается ')' после условия " + context);
    return condition;
}

}  // namespace bearlang
