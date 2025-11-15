#include "lexer.h"

#include <cctype>
#include <sstream>
#include <unordered_map>

namespace bearlang {

namespace {
const std::unordered_map<std::string, TokenType> kKeywords = {
    {"целое", TokenType::KeywordInteger},
    {"дробное", TokenType::KeywordDouble},
    {"строка", TokenType::KeywordString},
    {"логика", TokenType::KeywordLogic},
    {"если", TokenType::KeywordIf},
    {"иначе", TokenType::KeywordElse},
    {"пока", TokenType::KeywordWhile},
    {"для", TokenType::KeywordFor},
    {"ввод", TokenType::KeywordInput},
    {"вывод", TokenType::KeywordOutput},
    {"и", TokenType::KeywordAnd},
    {"или", TokenType::KeywordOr},
    {"не", TokenType::KeywordNot},
    {"от", TokenType::KeywordFrom},
    {"до", TokenType::KeywordTo},
    {"правда", TokenType::KeywordTrue},
    {"ложь", TokenType::KeywordFalse}};
}

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    while (current_ < source_.size()) {
        if (atLineStart_) {
            std::size_t temp = current_;
            std::size_t indentCount = 0;
            while (temp < source_.size()) {
                char ch = source_[temp];
                if (ch == ' ') {
                    ++indentCount;
                    ++temp;
                } else if (ch == '\t') {
                    indentCount += 4;
                    ++temp;
                } else if (ch == '\r') {
                    ++temp;
                } else {
                    break;
                }
            }

            if (temp >= source_.size()) {
                current_ = temp;
                break;
            }

            char next = source_[temp];
            if (next == '\n') {
                current_ = temp;
                column_ = 1;
            } else if (next == '/' && temp + 1 < source_.size() && source_[temp + 1] == '/') {
                current_ = temp;
                column_ = indentCount + 1;
                skipComment();
                continue;
            } else {
                handleIndentation(indentCount, line_);
                current_ = temp;
                column_ = indentCount + 1;
                atLineStart_ = false;
            }
        }

        if (current_ >= source_.size()) {
            break;
        }

        char ch = source_[current_];

        if (ch == ' ' || ch == '\t') {
            ++current_;
            ++column_;
            continue;
        }

        if (ch == '\r') {
            ++current_;
            continue;
        }

        if (ch == '\n') {
            pushToken(TokenType::Newline);
            ++current_;
            ++line_;
            column_ = 1;
            atLineStart_ = true;
            continue;
        }

        if (ch == '/' && peekChar(1) == '/') {
            skipComment();
            continue;
        }

        if (ch == '"') {
            scanString();
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch))) {
            scanNumber();
            continue;
        }

        if (isIdentifierStart(ch)) {
            scanIdentifierOrKeyword();
            continue;
        }

        scanOperator();
    }

    emitPendingDedents(line_);
    pushToken(TokenType::EndOfFile);
    return tokens_;
}

void Lexer::pushToken(TokenType type, const std::string& lexeme) {
    tokens_.push_back(Token{type, lexeme, line_, column_});
}

void Lexer::handleIndentation(std::size_t spaces, std::size_t line) {
    if (spaces > indentStack_.back()) {
        indentStack_.push_back(spaces);
        tokens_.push_back(Token{TokenType::Indent, "", line, 1});
    } else {
        while (spaces < indentStack_.back()) {
            indentStack_.pop_back();
            tokens_.push_back(Token{TokenType::Dedent, "", line, 1});
        }
        if (spaces != indentStack_.back()) {
            std::ostringstream oss;
            oss << "Несогласованный отступ на строке " << line;
            throw LexerError(oss.str());
        }
    }
}

void Lexer::emitPendingDedents(std::size_t line) {
    while (indentStack_.size() > 1) {
        indentStack_.pop_back();
        tokens_.push_back(Token{TokenType::Dedent, "", line, 1});
    }
}

void Lexer::skipComment() {
    while (current_ < source_.size() && source_[current_] != '\n') {
        ++current_;
        ++column_;
    }
}

char Lexer::peekChar(std::size_t offset) const {
    std::size_t index = current_ + offset;
    if (index >= source_.size()) {
        return '\0';
    }
    return source_[index];
}
bool Lexer::isIdentifierStart(char ch) const {
    unsigned char u = static_cast<unsigned char>(ch);
    return std::isalpha(u) || ch == '_' || u >= 128;
}

bool Lexer::isIdentifierPart(char ch) const {
    unsigned char u = static_cast<unsigned char>(ch);
    return std::isalnum(u) || ch == '_' || u >= 128;
}

void Lexer::scanIdentifierOrKeyword() {
    std::size_t start = current_;
    std::size_t startColumn = column_;
    while (current_ < source_.size() && isIdentifierPart(source_[current_])) {
        ++current_;
        ++column_;
    }
    std::string text = source_.substr(start, current_ - start);
    auto it = kKeywords.find(text);
    TokenType type = it == kKeywords.end() ? TokenType::Identifier : it->second;
    tokens_.push_back(Token{type, text, line_, startColumn});
}

void Lexer::scanNumber() {
    std::size_t start = current_;
    std::size_t startColumn = column_;
    bool seenDot = false;
    while (current_ < source_.size()) {
        char ch = source_[current_];
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            ++current_;
            ++column_;
            continue;
        }
        if (ch == '.' && !seenDot) {
            char next = peekChar(1);
            if (!std::isdigit(static_cast<unsigned char>(next))) {
                break;
            }
            seenDot = true;
            ++current_;
            ++column_;
            continue;
        }
        break;
    }
    std::string number = source_.substr(start, current_ - start);
    TokenType type = seenDot ? TokenType::DoubleLiteral : TokenType::IntegerLiteral;
    tokens_.push_back(Token{type, number, line_, startColumn});
}

void Lexer::scanString() {
    std::size_t startColumn = column_;
    ++current_;  // Skip opening quote
    ++column_;
    std::string value;
    while (current_ < source_.size()) {
        char ch = source_[current_];
        if (ch == '\n') {
            throw LexerError("Строковый литерал не может переноситься на новую строку");
        }
        if (ch == '"') {
            ++current_;
            ++column_;
            tokens_.push_back(Token{TokenType::StringLiteral, value, line_, startColumn});
            return;
        }
        if (ch == '\\') {
            ++current_;
            ++column_;
            if (current_ >= source_.size()) {
                throw LexerError("Незавершённая escape-последовательность");
            }
            char next = source_[current_];
            switch (next) {
                case '\\': value.push_back('\\'); break;
                case '"': value.push_back('"'); break;
                case 'n': value.push_back('\n'); break;
                case 't': value.push_back('\t'); break;
                default:
                    throw LexerError("Неизвестная escape-последовательность");
            }
            ++current_;
            ++column_;
            continue;
        }
        value.push_back(ch);
        ++current_;
        ++column_;
    }
    throw LexerError("Незакрытая строка");
}

void Lexer::scanOperator() {
    char ch = source_[current_];
    switch (ch) {
        case '+':
            pushToken(TokenType::Plus);
            ++current_;
            ++column_;
            break;
        case '-':
            pushToken(TokenType::Minus);
            ++current_;
            ++column_;
            break;
        case '*':
            pushToken(TokenType::Star);
            ++current_;
            ++column_;
            break;
        case '/':
            pushToken(TokenType::Slash);
            ++current_;
            ++column_;
            break;
        case '%':
            pushToken(TokenType::Percent);
            ++current_;
            ++column_;
            break;
        case '^':
            pushToken(TokenType::Caret);
            ++current_;
            ++column_;
            break;
        case '(':
            pushToken(TokenType::LeftParen);
            ++current_;
            ++column_;
            break;
        case ')':
            pushToken(TokenType::RightParen);
            ++current_;
            ++column_;
            break;
        case ',':
            pushToken(TokenType::Comma);
            ++current_;
            ++column_;
            break;
        case '=':
            if (peekChar(1) == '=') {
                pushToken(TokenType::Equal);
                current_ += 2;
                column_ += 2;
            } else {
                pushToken(TokenType::Assign);
                ++current_;
                ++column_;
            }
            break;
        case '<':
            if (peekChar(1) == '=') {
                pushToken(TokenType::LessEqual);
                current_ += 2;
                column_ += 2;
            } else {
                pushToken(TokenType::Less);
                ++current_;
                ++column_;
            }
            break;
        case '>':
            if (peekChar(1) == '=') {
                pushToken(TokenType::GreaterEqual);
                current_ += 2;
                column_ += 2;
            } else {
                pushToken(TokenType::Greater);
                ++current_;
                ++column_;
            }
            break;
        default: {
            std::ostringstream oss;
            oss << "Неизвестный символ '" << ch << "' на строке " << line_ << ":" << column_;
            throw LexerError(oss.str());
        }
    }
}

}  // namespace bearlang
