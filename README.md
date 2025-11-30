# BearLang Classroom

BearLang Classroom is a tiny C++ application that helps children take their first programming steps. Kids write short BearLang scripts (a friendly, Russian-like language), see the generated C++ code, and immediately run the compiled result.

## Task Description
The tool provides an end-to-end playground: read a BearLang script, turn it into an abstract syntax tree (AST), produce human-readable C++20, and invoke `g++` so learners can observe both the generated source and runtime output. The repository contains all compiler stages (lexer → parser → code generator) plus a CLI wrapper that loads `examples/*.txt`, reports translation errors in Russian, and streams program input/output through the same terminal session.

## Functionality Formalization
**Supported**
- Source format: UTF-8 BearLang text files with indentation-based blocks (tabs count as four spaces) and optional `//` comments.
- Types and literals: `целое`, `дробное`, `строка`, `логика`; integer, floating-point, quoted string (with `\"`, `\\`, `\n`, `\t` escapes), and boolean literals `правда` / `ложь`.
- Statements: typed variable declarations with optional initializer, reassignment of existing identifiers, `ввод name`/`вывод expr`, `если`/`иначе если`/`иначе`, `пока (условие)`, and `для (<тип> i от expr до expr)` with inclusive upper bound and step `+1`.
- Expressions: parentheses, identifier references, unary `-` / `не`, arithmetic `+ - * / % ^`, comparisons (`< <= > >= ==`), and logical `и` / `или`. Power maps to `std::pow`, other operators translate directly to C++.
- Code generation: wraps statements inside `int main()`, injects standard headers, ensures scoped variable mangling per block, and emits readable, indented C++ that is immediately compiled via the CLI helper.

**Not supported**
- User-defined functions, procedures, or return statements; every script is a straight-line `main`.
- Collections, arrays, structs, type inference, or automatic conversions beyond what C++ accepts for the emitted code.
- Loop modifiers (`break`, `continue`), custom step sizes in `для`, or decremented ranges.
- Multi-line or single-quoted strings, raw string literals, and multi-line comments.
- Advanced I/O (`форматированный вывод`, files) and runtime libraries outside `iostream`, `cmath`, and `string`.

## Features
- Tokenizer, parser, and code generator tailored to BearLang.
- Translation of BearLang programs into readable C++20 code.
- One-button runner: compile the generated C++ with `g++` and show the output.
- Starter library of examples (`examples/*.txt`).
- Helpful error messages when the code cannot be parsed.

## BearLang Cheatsheet
| BearLang | Meaning |
| --- | --- |
| `целое`, `дробное`, `строка`, `логика` | `int`, `double`, `std::string`, `bool` |
| `ввод a` | `std::cin >> a;` |
| `вывод expr` | `std::cout << expr << std::endl;` |
| `если`, `иначе если`, `иначе` | `if`, `else if`, `else` |
| `пока (условие)` | `while (condition)` |
| `для (целое i от 0 до 5)` | `for (int i = 0; i <= 5; ++i)` |
| `правда`, `ложь` | `true`, `false` |
| `и`, `или`, `не` | `&&`, `||`, `!` |
| `+ - * / % ^` | math operators (`^` becomes `std::pow`) |

Blocks are indentation-based, similar to Python. Each nested block must be indented consistently (tabs or four spaces).

## Building
```bash
cd bearlang-app
cmake -S . -B build
cmake --build build
```

## Running the Playground
```bash
./build/bearlang_app
```
Then:
1. Pick one of the bundled examples **or** type the path to your own `.txt` file.
2. The translator shows where the generated C++ file lives.
3. The program is compiled with `g++ -std=c++20` and executed; provide any required input directly in the same terminal.

## Adding New Lessons
1. Drop a new `.txt` script under `examples/`.
2. Teach new syntax by extending the lexer (`app/core/lexer`), parser (`app/core/parser`), and code generator (`app/core/codegen`).
3. Rebuild the tool—no other setup is required.

Have fun exploring C++ through BearLang!
