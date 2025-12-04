#include "codegen.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace bearlang {

namespace {

class NameMangler {
public:
    NameMangler() {
        scopes_.emplace_back();
    }

    void pushScope() {
        scopes_.emplace_back();
    }

    void popScope() {
        if (scopes_.size() > 1) {
            scopes_.pop_back();
        }
    }

    std::string declare(const std::string& original) {
        std::string renamed = "vr_" + std::to_string(++counter_);
        scopes_.back()[original] = renamed;
        return renamed;
    }

    std::string resolve(const std::string& original) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(original);
            if (found != it->end()) {
                return found->second;
            }
        }
        return original;
    }

private:
    std::size_t counter_ = 0;
    std::vector<std::unordered_map<std::string, std::string>> scopes_;
};

std::string indent(std::size_t level) {
    return std::string(level * 4, ' ');
}

std::string cppType(ValueType type) {
    switch (type) {
        case ValueType::Integer: return "int";
        case ValueType::Double: return "double";
        case ValueType::String: return "std::string";
        case ValueType::Boolean: return "bool";
        case ValueType::Unknown: default: return "auto";
    }
}

std::string escapeString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size()+2);
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string emitExpression(const ExprPtr& expr, const NameMangler& mangler);
void emitStatements(const std::vector<StmtPtr>& statements,
                    std::size_t indentLevel,
                    std::ostringstream& out,
                    NameMangler& mangler,
                    bool createNewScope);

void emitStatement(const Statement& statement,
                   std::size_t indentLevel,
                   std::ostringstream& out,
                   NameMangler& mangler) {
    switch (statement.kind()) {
        case StatementKind::VarDecl: {
            const auto& decl = static_cast<const VarDeclStmt&>(statement);
            const std::string cppName = mangler.declare(decl.name);
            out << indent(indentLevel) << cppType(decl.type) << " " << cppName;
            if (decl.initializer) {
                out << " = " << emitExpression(decl.initializer, mangler);
            } else {
                out << "{}";
            }
            out << ";\n";
            break;
        }
        case StatementKind::Assign: {
            const auto& assign = static_cast<const AssignStmt&>(statement);
            out << indent(indentLevel) << mangler.resolve(assign.name) << " = "
                << emitExpression(assign.value, mangler) << ";\n";
            break;
        }
        case StatementKind::Input: {
            const auto& input = static_cast<const InputStmt&>(statement);
            out << indent(indentLevel) << "std::cin >> " << mangler.resolve(input.name) << ";\n";
            break;
        }
        case StatementKind::Output: {
            const auto& outputStmt = static_cast<const OutputStmt&>(statement);
            out << indent(indentLevel) << "std::cout << "
                << emitExpression(outputStmt.value, mangler) << " << std::endl;\n";
            break;
        }
        case StatementKind::If: {
            const auto& ifStmt = static_cast<const IfStmt&>(statement);
            for (std::size_t i = 0; i < ifStmt.branches.size(); ++i) {
                const auto& branch = ifStmt.branches[i];
                out << indent(indentLevel) << (i == 0 ? "if" : "else if") << " ("
                    << emitExpression(branch.condition, mangler) << ") {\n";
                emitStatements(branch.body, indentLevel + 1, out, mangler, true);
                out << indent(indentLevel) << "}\n";
            }
            if (ifStmt.hasElse) {
                out << indent(indentLevel) << "else {\n";
                emitStatements(ifStmt.elseBranch, indentLevel + 1, out, mangler, true);
                out << indent(indentLevel) << "}\n";
            }
            break;
        }
        case StatementKind::While: {
            const auto& loop = static_cast<const WhileStmt&>(statement);
            out << indent(indentLevel) << "while (" << emitExpression(loop.condition, mangler)
                << ") {\n";
            emitStatements(loop.body, indentLevel + 1, out, mangler, true);
            out << indent(indentLevel) << "}\n";
            break;
        }
        case StatementKind::ForRange: {
            const auto& loop = static_cast<const ForRangeStmt&>(statement);
            mangler.pushScope();
            const std::string loopName = mangler.declare(loop.name);
            out << indent(indentLevel) << "for (" << cppType(loop.type) << " " << loopName << " = "
                << emitExpression(loop.from, mangler) << "; " << loopName << " <= "
                << emitExpression(loop.to, mangler) << "; ++" << loopName << ") {\n";
            emitStatements(loop.body, indentLevel + 1, out, mangler, true);
            out << indent(indentLevel) << "}\n";
            mangler.popScope();
            break;
        }
    }
}

std::string emitExpression(const ExprPtr& expr, const NameMangler& mangler) {
    if (!expr) {
        return "0";
    }
    switch (expr->kind()) {
        case ExpressionKind::Literal: {
            const auto& literal = static_cast<const LiteralExpr&>(*expr);
            switch (literal.type) {
                case ValueType::Integer:
                case ValueType::Double:
                    return literal.text;
                case ValueType::String:
                    return std::string("\"") + escapeString(literal.text) + "\"";
                case ValueType::Boolean:
                    return literal.boolValue ? std::string("true") : std::string("false");
                case ValueType::Unknown:
                default:
                    return literal.text;
            }
        }
        case ExpressionKind::Variable: {
            const auto& var = static_cast<const VariableExpr&>(*expr);
            return mangler.resolve(var.name);
        }
        case ExpressionKind::Unary: {
            const auto& unary = static_cast<const UnaryExpr&>(*expr);
            return unary.op + "(" + emitExpression(unary.operand, mangler) + ")";
        }
        case ExpressionKind::Binary: {
            const auto& binary = static_cast<const BinaryExpr&>(*expr);
            if (binary.op == "^") {
                return std::string("std::pow(") + emitExpression(binary.left, mangler) + ", " +
                       emitExpression(binary.right, mangler) + ")";
            }
            return std::string("(") + emitExpression(binary.left, mangler) + " " + binary.op + " " +
                   emitExpression(binary.right, mangler) + ")";
        }
    }
    return {};
}

void emitStatements(const std::vector<StmtPtr>& statements,
                    std::size_t indentLevel,
                    std::ostringstream& out,
                    NameMangler& mangler,
                    bool createNewScope) {
    if (createNewScope) {
        mangler.pushScope();
    }
    for (const auto& stmt : statements) {
        emitStatement(*stmt, indentLevel, out, mangler);
    }
    if (createNewScope) {
        mangler.popScope();
    }
}

}  // namespace

std::string CodeGenerator::generate(const Program& program) {
    std::ostringstream out;
    NameMangler mangler;
    out << "#include <cmath>\n";
    out << "#include <iostream>\n";
    out << "#include <string>\n\n";
    out << "int main() {\n";
    out << indent(1) << "std::ios_base::sync_with_stdio(false);\n";
    //out << indent(1) << "std::cin.tie(nullptr);\n";
    //out << indent(1) << "std::cout << std::boolalpha;\n";
    emitStatements(program.statements, 1, out, mangler, false);
    out << indent(1) << "return 0;\n";
    out << "}\n";
    return out.str();
}

}  // namespace bearlang
