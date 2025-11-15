#include "codegen.h"

#include <sstream>
#include <string>

namespace bearlang {

namespace {

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

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

std::string emitExpression(const ExprPtr& expr);
void emitStatements(const std::vector<StmtPtr>& statements, std::size_t indentLevel, std::ostringstream& out);

void emitStatement(const Statement& statement, std::size_t indentLevel, std::ostringstream& out) {
    std::visit(Overloaded{
                   [&](const Statement::VarDecl& decl) {
                       out << indent(indentLevel) << cppType(decl.type) << " " << decl.name;
                       if (decl.initializer) {
                           out << " = " << emitExpression(decl.initializer);
                       } else {
                           out << "{}";
                       }
                       out << ";\n";
                   },
                   [&](const Statement::Assign& assign) {
                       out << indent(indentLevel) << assign.name << " = "
                           << emitExpression(assign.value) << ";\n";
                   },
                   [&](const Statement::Input& input) {
                       out << indent(indentLevel) << "std::cin >> " << input.name << ";\n";
                   },
                   [&](const Statement::Output& output) {
                       out << indent(indentLevel) << "std::cout << "
                           << emitExpression(output.value) << " << std::endl;\n";
                   },
                   [&](const Statement::If& ifStmt) {
                       for (std::size_t i = 0; i < ifStmt.branches.size(); ++i) {
                           const auto& branch = ifStmt.branches[i];
                           out << indent(indentLevel) << (i == 0 ? "if" : "else if")
                               << " (" << emitExpression(branch.condition) << ") {\n";
                           emitStatements(branch.body, indentLevel + 1, out);
                           out << indent(indentLevel) << "}\n";
                       }
                       if (ifStmt.hasElse) {
                           out << indent(indentLevel) << "else {\n";
                           emitStatements(ifStmt.elseBranch, indentLevel + 1, out);
                           out << indent(indentLevel) << "}\n";
                       }
                   },
                   [&](const Statement::WhileLoop& loop) {
                       out << indent(indentLevel) << "while (" << emitExpression(loop.condition)
                           << ") {\n";
                       emitStatements(loop.body, indentLevel + 1, out);
                       out << indent(indentLevel) << "}\n";
                   },
                   [&](const Statement::ForRange& loop) {
                       out << indent(indentLevel) << "for (" << cppType(loop.type) << " "
                           << loop.name << " = " << emitExpression(loop.from) << "; "
                           << loop.name << " <= " << emitExpression(loop.to) << "; ++"
                           << loop.name << ") {\n";
                       emitStatements(loop.body, indentLevel + 1, out);
                       out << indent(indentLevel) << "}\n";
                   }},
               statement.node);
}

std::string emitExpression(const ExprPtr& expr) {
    if (!expr) {
        return "0";
    }
    return std::visit(
        Overloaded{
            [](const Expression::Literal& literal) {
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
            },
            [](const Expression::Variable& var) { return var.name; },
            [](const Expression::Unary& unary) {
                return unary.op + "(" + emitExpression(unary.operand) + ")";
            },
            [](const Expression::Binary& binary) {
                if (binary.op == "^") {
                    return std::string("std::pow(") + emitExpression(binary.left) + ", " +
                           emitExpression(binary.right) + ")";
                }
                return std::string("(") + emitExpression(binary.left) + " " + binary.op +
                       " " + emitExpression(binary.right) + ")";
            }},
        expr->node);
}

void emitStatements(const std::vector<StmtPtr>& statements, std::size_t indentLevel, std::ostringstream& out) {
    for (const auto& stmt : statements) {
        emitStatement(*stmt, indentLevel, out);
    }
}

}  // namespace

std::string CodeGenerator::generate(const Program& program) {
    std::ostringstream out;
    out << "#include <cmath>\n";
    out << "#include <iostream>\n";
    out << "#include <string>\n\n";
    out << "int main() {\n";
    out << indent(1) << "std::ios_base::sync_with_stdio(false);\n";
    out << indent(1) << "std::cin.tie(nullptr);\n";
    out << indent(1) << "std::cout << std::boolalpha;\n";
    emitStatements(program.statements, 1, out);
    out << indent(1) << "return 0;\n";
    out << "}\n";
    return out.str();
}

}  // namespace bearlang
