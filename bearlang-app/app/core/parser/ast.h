#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bearlang {

enum class ValueType { Integer, Double, String, Boolean, Unknown };

struct Expression;
struct Statement;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;

enum class ExpressionKind { Literal, Variable, Unary, Binary };

struct Expression {
    explicit Expression(ExpressionKind kind) : kind_(kind) {}
    virtual ~Expression() = default;

    ExpressionKind kind() const { return kind_; }

private:
    ExpressionKind kind_;
};

struct LiteralExpr : Expression {
    LiteralExpr(ValueType valueType, std::string valueText, bool valueBool = false)
        : Expression(ExpressionKind::Literal),
          type(valueType),
          text(std::move(valueText)),
          boolValue(valueBool) {}

    ValueType type;
    std::string text;
    bool boolValue = false;
};

struct VariableExpr : Expression {
    explicit VariableExpr(std::string valueName)
        : Expression(ExpressionKind::Variable), name(std::move(valueName)) {}

    std::string name;
};

struct UnaryExpr : Expression {
    UnaryExpr(std::string valueOp, ExprPtr operandExpr)
        : Expression(ExpressionKind::Unary),
          op(std::move(valueOp)),
          operand(std::move(operandExpr)) {}

    std::string op;
    ExprPtr operand;
};

struct BinaryExpr : Expression {
    BinaryExpr(std::string valueOp, ExprPtr leftExpr, ExprPtr rightExpr)
        : Expression(ExpressionKind::Binary),
          op(std::move(valueOp)),
          left(std::move(leftExpr)),
          right(std::move(rightExpr)) {}

    std::string op;
    ExprPtr left;
    ExprPtr right;
};

inline ExprPtr makeLiteral(ValueType type, std::string text, bool boolValue = false) {
    return std::make_unique<LiteralExpr>(type, std::move(text), boolValue);
}

inline ExprPtr makeVariable(std::string name) {
    return std::make_unique<VariableExpr>(std::move(name));
}

inline ExprPtr makeUnary(std::string op, ExprPtr operand) {
    return std::make_unique<UnaryExpr>(std::move(op), std::move(operand));
}

inline ExprPtr makeBinary(std::string op, ExprPtr left, ExprPtr right) {
    return std::make_unique<BinaryExpr>(std::move(op), std::move(left), std::move(right));
}

enum class StatementKind { VarDecl, Assign, Input, Output, If, While, ForRange };

struct Statement {
    explicit Statement(StatementKind kind) : kind_(kind) {}
    virtual ~Statement() = default;

    StatementKind kind() const { return kind_; }

private:
    StatementKind kind_;
};

struct VarDeclStmt : Statement {
    VarDeclStmt(ValueType valueType, std::string valueName, ExprPtr init)
        : Statement(StatementKind::VarDecl),
          type(valueType),
          name(std::move(valueName)),
          initializer(std::move(init)) {}

    ValueType type;
    std::string name;
    ExprPtr initializer;
};

struct AssignStmt : Statement {
    AssignStmt(std::string valueName, ExprPtr exprValue)
        : Statement(StatementKind::Assign),
          name(std::move(valueName)),
          value(std::move(exprValue)) {}

    std::string name;
    ExprPtr value;
};

struct InputStmt : Statement {
    explicit InputStmt(std::string valueName)
        : Statement(StatementKind::Input), name(std::move(valueName)) {}

    std::string name;
};

struct OutputStmt : Statement {
    explicit OutputStmt(ExprPtr exprValue)
        : Statement(StatementKind::Output), value(std::move(exprValue)) {}

    ExprPtr value;
};

struct IfStmt : Statement {
    struct Branch {
        ExprPtr condition;
        std::vector<StmtPtr> body;
    };

    IfStmt() : Statement(StatementKind::If) {}

    std::vector<Branch> branches;
    std::vector<StmtPtr> elseBranch;
    bool hasElse = false;
};

struct WhileStmt : Statement {
    WhileStmt(ExprPtr loopCondition, std::vector<StmtPtr> loopBody)
        : Statement(StatementKind::While),
          condition(std::move(loopCondition)),
          body(std::move(loopBody)) {}

    ExprPtr condition;
    std::vector<StmtPtr> body;
};

struct ForRangeStmt : Statement {
    ForRangeStmt(ValueType valueType,
                 std::string valueName,
                 ExprPtr rangeFrom,
                 ExprPtr rangeTo,
                 std::vector<StmtPtr> loopBody)
        : Statement(StatementKind::ForRange),
          type(valueType),
          name(std::move(valueName)),
          from(std::move(rangeFrom)),
          to(std::move(rangeTo)),
          body(std::move(loopBody)) {}

    ValueType type;
    std::string name;
    ExprPtr from;
    ExprPtr to;
    std::vector<StmtPtr> body;
};

inline StmtPtr makeVarDecl(ValueType type, std::string name, ExprPtr initializer) {
    return std::make_unique<VarDeclStmt>(type, std::move(name), std::move(initializer));
}

inline StmtPtr makeAssign(std::string name, ExprPtr value) {
    return std::make_unique<AssignStmt>(std::move(name), std::move(value));
}

inline StmtPtr makeInput(std::string name) {
    return std::make_unique<InputStmt>(std::move(name));
}

inline StmtPtr makeOutput(ExprPtr value) {
    return std::make_unique<OutputStmt>(std::move(value));
}

inline StmtPtr makeWhile(ExprPtr condition, std::vector<StmtPtr> body) {
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

inline StmtPtr makeFor(ValueType type,
                       std::string name,
                       ExprPtr from,
                       ExprPtr to,
                       std::vector<StmtPtr> body) {
    return std::make_unique<ForRangeStmt>(
        type, std::move(name), std::move(from), std::move(to), std::move(body));
}

struct Program {
    std::vector<StmtPtr> statements;
};

}  // namespace bearlang
