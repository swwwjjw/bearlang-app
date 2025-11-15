#include "token.h"

namespace bearlang {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::EndOfFile: return "EOF";
        case TokenType::Newline: return "NEWLINE";
        case TokenType::Indent: return "INDENT";
        case TokenType::Dedent: return "DEDENT";
        case TokenType::Identifier: return "IDENT";
        case TokenType::IntegerLiteral: return "INT";
        case TokenType::DoubleLiteral: return "DOUBLE";
        case TokenType::StringLiteral: return "STRING";
        case TokenType::KeywordInteger: return "целое";
        case TokenType::KeywordDouble: return "дробное";
        case TokenType::KeywordString: return "строка";
        case TokenType::KeywordLogic: return "логика";
        case TokenType::KeywordIf: return "если";
        case TokenType::KeywordElse: return "иначе";
        case TokenType::KeywordWhile: return "пока";
        case TokenType::KeywordFor: return "для";
        case TokenType::KeywordInput: return "ввод";
        case TokenType::KeywordOutput: return "вывод";
        case TokenType::KeywordAnd: return "и";
        case TokenType::KeywordOr: return "или";
        case TokenType::KeywordNot: return "не";
        case TokenType::KeywordFrom: return "от";
        case TokenType::KeywordTo: return "до";
        case TokenType::KeywordTrue: return "правда";
        case TokenType::KeywordFalse: return "ложь";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Caret: return "^";
        case TokenType::Assign: return "=";
        case TokenType::Equal: return "==";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::Comma: return ",";
    }
    return "?";
}

}  // namespace bearlang
