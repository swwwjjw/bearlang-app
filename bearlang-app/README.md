# BearLang Classroom

BearLang Classroom is a tiny C++ application that helps children take their first programming steps. Kids write short BearLang scripts (a friendly, Russian-like language), see the generated C++ code, and immediately run the compiled result.

## Features
- Tokenizer, parser, and code generator tailored to BearLang.
- Translation of BearLang programs into readable C++20 code.
- One-button runner: compile the generated C++ with `g++` and show the output.
- Starter library of examples (`examples/*.bear`).
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
1. Pick one of the bundled examples **or** type the path to your own `.bear` file.
2. The translator shows where the generated C++ file lives.
3. The program is compiled with `g++ -std=c++20` and executed; provide any required input directly in the same terminal.

## Adding New Lessons
1. Drop a new `.bear` script under `examples/`.
2. Teach new syntax by extending the lexer (`app/core/lexer`), parser (`app/core/parser`), and code generator (`app/core/codegen`).
3. Rebuild the tool—no other setup is required.

Have fun exploring C++ through BearLang!
