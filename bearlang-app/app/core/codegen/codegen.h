#pragma once

#include <string>

#include "core/parser/ast.h"

namespace bearlang {

class CodeGenerator {
public:
    static std::string generate(const Program& program);
};

}  // namespace bearlang
