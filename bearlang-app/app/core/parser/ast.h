#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace bearlang {

enum class ValueType { Integer, Double, String, Boolean, Unknown };

struct Expression;
struct Statement;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;

struct Expression {
    struct Literal {
        ValueType type;
        std::string text;
        bool boolValue = false;
    };

    struct Variable {
        std::string name;
    };

    struct Unary {
        std::string op;
        ExprPtr operand;
    };

    struct Binary {
        std::string op;
        ExprPtr left;
        ExprPtr right;
    };

    using Variant = std::variant<Literal, Variable, Unary, Binary>;
    Variant node;

    explicit Expression(Variant variant) : node(std::move(variant)) {}
};

inline ExprPtr makeLiteral(ValueType type, std::string text, bool boolValue = false) {
    return std::make_unique<Expression>(
        Expression{Expression::Literal{type, std::move(text), boolValue}});
}

inline ExprPtr makeVariable(std::string name) {
    return std::make_unique<Expression>(Expression{Expression::Variable{std::move(name)}});
}

inline ExprPtr makeUnary(std::string op, ExprPtr operand) {
    return std::make_unique<Expression>(
        Expression{Expression::Unary{std::move(op), std::move(operand)}});
}

inline ExprPtr makeBinary(std::string op, ExprPtr left, ExprPtr right) {
    return std::make_unique<Expression>(
        Expression{Expression::Binary{std::move(op), std::move(left), std::move(right)}});
}

struct Statement {
    struct VarDecl {
        ValueType type;
        std::string name;
        ExprPtr initializer;
    };

    struct Assign {
        std::string name;
        ExprPtr value;
    };

    struct Input {
        std::string name;
    };

    struct Output {
        ExprPtr value;
    };

    struct IfBranch {
        ExprPtr condition;
        std::vector<StmtPtr> body;
    };

    struct If {
        std::vector<IfBranch> branches;
        std::vector<StmtPtr> elseBranch;
        bool hasElse = false;
    };

    struct WhileLoop {
        ExprPtr condition;
        std::vector<StmtPtr> body;
    };

    struct ForRange {
        ValueType type;
        std::string name;
        ExprPtr from;
        ExprPtr to;
        std::vector<StmtPtr> body;
    };

    using Variant = std::variant<VarDecl, Assign, Input, Output, If, WhileLoop, ForRange>;
    Variant node;

    explicit Statement(Variant variant) : node(std::move(variant)) {}
};

inline StmtPtr makeVarDecl(ValueType type, std::string name, ExprPtr initializer) {
    return std::make_unique<Statement>(
        Statement{Statement::VarDecl{type, std::move(name), std::move(initializer)}} );
}

inline StmtPtr makeAssign(std::string name, ExprPtr value) {
    return std::make_unique<Statement>(
        Statement{Statement::Assign{std::move(name), std::move(value)}} );
}

inline StmtPtr makeInput(std::string name) {
    return std::make_unique<Statement>(Statement{Statement::Input{std::move(name)}} );
}

inline StmtPtr makeOutput(ExprPtr value) {
    return std::make_unique<Statement>(Statement{Statement::Output{std::move(value)}} );
}

inline StmtPtr makeWhile(ExprPtr condition, std::vector<StmtPtr> body) {
    return std::make_unique<Statement>(
        Statement{Statement::WhileLoop{std::move(condition), std::move(body)}} );
}

inline StmtPtr makeFor(ValueType type, std::string name, ExprPtr from, ExprPtr to, std::vector<StmtPtr> body) {
    return std::make_unique<Statement>(Statement{Statement::ForRange{
        type, std::move(name), std::move(from), std::move(to), std::move(body)}} );
}

struct Program {
    std::vector<StmtPtr> statements;
};

}  // namespace bearlang
