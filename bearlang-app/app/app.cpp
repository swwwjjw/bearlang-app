#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "core/codegen/codegen.h"
#include "core/lexer/lexer.h"
#include "core/parser/parser.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using bearlang::CodeGenerator;
using bearlang::Lexer;
using bearlang::Parser;

fs::path executableDir() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return fs::current_path();
    }
    buffer.resize(length);
    return fs::path(buffer).parent_path();
#else
    std::vector<char> buffer(1024);
    while (true) {
        ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
        if (length < 0) {
            return fs::current_path();
        }
        if (static_cast<std::size_t>(length) < buffer.size() - 1) {
            buffer[length] = '\0';
            return fs::path(buffer.data()).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
}

fs::path findProjectRoot(const fs::path& startDir) {
    fs::path current = startDir;
    while (!current.empty()) {
        if (fs::exists(current / "examples")) {
            return current;
        }
        const auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    return startDir;
}

std::string readAll(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Не удалось открыть файл: " + path.string());
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::vector<fs::path> loadExamples(const fs::path& examplesDir) {
    std::vector<fs::path> files;
    if (!fs::exists(examplesDir)) {
        return files;
    }
    for (const auto& entry : fs::directory_iterator(examplesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".txt") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

bool compileAndRun(const std::string& cppSource, const fs::path& workspace) {
    fs::create_directories(workspace);
    fs::path cppPath = workspace / "generated_program.cpp";
    fs::path exePath = workspace / "generated_program";
#ifdef _WIN32
    exePath += ".exe";
#endif

    std::ofstream out(cppPath);
    out << cppSource;
    out.close();

    std::cout << "C++ код сохранён в: " << cppPath << "\n";

    std::string compileCommand = "g++ -std=c++03 \"" + cppPath.string() + "\" -o \"" + exePath.string() + "\"";
    std::cout << "Компиляция...\n";
    int compileResult = std::system(compileCommand.c_str());
    if (compileResult != 0) {
        std::cerr << "Компилятор вернул ошибку." << std::endl;
        return false;
    }

    std::cout << "\n--- Результат программы ---\n";
#ifdef _WIN32
    std::string runCommand = "\"" + exePath.string() + "\"";
#else
    std::string runCommand = "\"" + exePath.string() + "\"";
#endif
    int runResult = std::system(runCommand.c_str());
    std::cout << "\n---------------------------\n";
    return runResult == 0;
}

bool translateAndRun(const fs::path& sourcePath, const fs::path& workspace) {
    try {
        std::string source = readAll(sourcePath);
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        bearlang::Program program = parser.parseProgram();
        std::string cppSource = CodeGenerator::generate(program);
        return compileAndRun(cppSource, workspace);
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
        return false;
    }
}

void printMenu() {
    std::cout << "BearLang Classroom" << std::endl;
    std::cout << "1. Запустить пример" << std::endl;
    std::cout << "2. Указать свой файл" << std::endl;
    std::cout << "3. Выход" << std::endl;
    std::cout << "Выбор: ";
}

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    fs::path root = findProjectRoot(executableDir());
    fs::path examplesDir = root / "examples";
    fs::path buildDir = root / "out";
    fs::create_directories(buildDir);

    std::cout << "Добро пожаловать! Напишите программу на BearLang и увидьте, как она превращается в C++." << std::endl;

    while (true) {
        printMenu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            break;
        }

        if (choice == "1") {
            auto examples = loadExamples(examplesDir);
            if (examples.empty()) {
                std::cout << "Примеры не найдены." << std::endl;
                continue;
            }
            std::cout << "Выберите пример:" << std::endl;
            for (std::size_t i = 0; i < examples.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << examples[i].filename().string() << std::endl;
            }
            std::cout << "Номер или пусто для отмены: ";
            std::string number;
            std::getline(std::cin, number);
            if (number.empty()) {
                continue;
            }
            std::size_t index = 0;
            try {
                index = std::stoul(number);
            } catch (const std::exception&) {
                std::cout << "Введите число." << std::endl;
                continue;
            }
            if (index == 0 || index > examples.size()) {
                std::cout << "Неверный номер." << std::endl;
                continue;
            }
            translateAndRun(examples[index - 1], buildDir);
        } else if (choice == "2") {
            std::cout << "Введите путь до .txt файла: ";
            std::string path;
            std::getline(std::cin, path);
            if (path.empty()) {
                continue;
            }
            fs::path userPath = fs::path(path);
            if (!fs::exists(userPath)) {
                std::cout << "Файл не найден." << std::endl;
                continue;
            }
            translateAndRun(userPath, buildDir);
        } else if (choice == "3" || choice == "q" || choice == "Q") {
            std::cout << "До новых встреч!" << std::endl;
            break;
        } else {
            std::cout << "Неизвестная команда." << std::endl;
        }
    }
    return 0;
}
