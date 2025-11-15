#include "app.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bear
{
	namespace
	{
		constexpr int kIndentSize = 4;

		const std::unordered_map<std::string, std::string> kTypeMap{
			{"целое", "int"},
			{"дробное", "double"},
			{"строка", "std::string"},
			{"логика", "bool"}};

		std::string trimCopy(const std::string& value)
		{
			std::size_t start = 0;
			while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
			{
				++start;
			}

			if (start == value.size())
			{
				return "";
			}

			std::size_t end = value.size() - 1;
			while (end > start && std::isspace(static_cast<unsigned char>(value[end])))
			{
				--end;
			}

			return value.substr(start, end - start + 1);
		}

		std::string stripComments(const std::string& line)
		{
			bool inString = false;
			for (std::size_t i = 0; i + 1 < line.size(); ++i)
			{
				char c = line[i];
				if (c == '"' && (i == 0 || line[i - 1] != '\\'))
				{
					inString = !inString;
				}

				if (!inString && c == '/' && line[i + 1] == '/')
				{
					return line.substr(0, i);
				}
			}

			return line;
		}

		int indentLevel(const std::string& line)
		{
			int spaces = 0;
			int level = 0;

			for (char c : line)
			{
				if (c == ' ')
				{
					++spaces;
					if (spaces == kIndentSize)
					{
						++level;
						spaces = 0;
					}
				}
				else if (c == '\t')
				{
					++level;
					spaces = 0;
				}
				else
				{
					break;
				}
			}

			if (spaces > 0)
			{
				++level;
			}

			return std::max(level, 0);
		}

		std::string indentation(int logicalIndent)
		{
			return std::string((logicalIndent + 1) * kIndentSize, ' ');
		}

		bool beginsWithWord(const std::string& text, const std::string& word)
		{
			if (text.size() < word.size())
			{
				return false;
			}

			if (text.compare(0, word.size(), word) != 0)
			{
				return false;
			}

			if (text.size() == word.size())
			{
				return true;
			}

			char next = text[word.size()];
			return std::isspace(static_cast<unsigned char>(next)) || next == '(';
		}

		bool isWordBoundary(char c)
		{
			return std::isspace(static_cast<unsigned char>(c)) || c == '(' || c == ')' || c == '[' || c == ']' ||
			       c == '{' || c == '}' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' ||
			       c == '!' || c == '=' || c == '<' || c == '>' || c == ',' || c == ';' || c == ':';
		}

		void replaceWord(std::string& text, const std::string& from, const std::string& to)
		{
			std::size_t pos = 0;
			while ((pos = text.find(from, pos)) != std::string::npos)
			{
				bool boundaryBefore = pos == 0 || isWordBoundary(text[pos - 1]);
				bool boundaryAfter = pos + from.size() >= text.size() || isWordBoundary(text[pos + from.size()]);
				if (boundaryBefore && boundaryAfter)
				{
					text.replace(pos, from.size(), to);
					pos += to.size();
				}
				else
				{
					pos += from.size();
				}
			}
		}

		bool isOperatorSymbol(char c)
		{
			switch (c)
			{
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '^':
			case '&':
			case '|':
			case '!':
			case '=':
			case '<':
			case '>':
			case '?':
			case ':':
			case ',':
				return true;
			default:
				return false;
			}
		}

		std::size_t skipSpacesBackward(const std::string& text, std::size_t index)
		{
			while (index > 0 && std::isspace(static_cast<unsigned char>(text[index - 1])))
			{
				--index;
			}
			return index;
		}

		std::size_t skipSpacesForward(const std::string& text, std::size_t index)
		{
			while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])))
			{
				++index;
			}
			return index;
		}

		std::pair<std::size_t, std::size_t> extractOperandBounds(const std::string& text,
		                                                         std::size_t caretPos,
		                                                         bool searchLeft,
		                                                         std::vector<std::string>& errors,
		                                                         std::size_t lineNumber)
		{
			if (searchLeft)
			{
				if (caretPos == 0)
				{
					errors.emplace_back("Строка " + std::to_string(lineNumber) +
					                    ": отсутствует левая часть для оператора '^'.");
					return {0, 0};
				}

				std::size_t end = skipSpacesBackward(text, caretPos);
				if (end == 0)
				{
					errors.emplace_back("Строка " + std::to_string(lineNumber) +
					                    ": отсутствует левая часть для оператора '^'.");
					return {0, 0};
				}

				char previous = text[end - 1];
				if (previous == ')')
				{
					int depth = 1;
					for (std::size_t i = end - 1; i-- > 0;)
					{
						if (text[i] == ')')
						{
							++depth;
						}
						else if (text[i] == '(')
						{
							--depth;
							if (depth == 0)
							{
								return {i, end};
							}
						}
					}

					errors.emplace_back("Строка " + std::to_string(lineNumber) +
					                    ": не удалось найти начало выражения перед '^'.");
					return {0, 0};
				}

				std::size_t start = end;
				while (start > 0)
				{
					char c = text[start - 1];
					if (std::isspace(static_cast<unsigned char>(c)) || isOperatorSymbol(c))
					{
						break;
					}
					--start;
				}

				return {start, end};
			}

			std::size_t start = skipSpacesForward(text, caretPos + 1);
			if (start >= text.size())
			{
				errors.emplace_back("Строка " + std::to_string(lineNumber) +
				                    ": отсутствует правая часть для оператора '^'.");
				return {text.size(), text.size()};
			}

			if (text[start] == '(')
			{
				int depth = 1;
				for (std::size_t i = start + 1; i < text.size(); ++i)
				{
					if (text[i] == '(')
					{
						++depth;
					}
					else if (text[i] == ')')
					{
						--depth;
						if (depth == 0)
						{
							return {start, i + 1};
						}
					}
				}

				errors.emplace_back("Строка " + std::to_string(lineNumber) +
				                    ": не удалось найти окончание выражения после '^'.");
				return {start, start};
			}

			std::size_t end = start;
			while (end < text.size())
			{
				char c = text[end];
				if (std::isspace(static_cast<unsigned char>(c)) || isOperatorSymbol(c))
				{
					break;
				}
				++end;
			}

			return {start, end};
		}

		std::string convertExponent(std::string text, std::vector<std::string>& errors, std::size_t lineNumber)
		{
			std::size_t pos = 0;
			while ((pos = text.find('^', pos)) != std::string::npos)
			{
				auto leftBounds = extractOperandBounds(text, pos, true, errors, lineNumber);
				auto rightBounds = extractOperandBounds(text, pos, false, errors, lineNumber);
				if (leftBounds.first == leftBounds.second || rightBounds.first == rightBounds.second)
				{
					break;
				}

				std::string left = trimCopy(text.substr(leftBounds.first, leftBounds.second - leftBounds.first));
				std::string right = trimCopy(text.substr(rightBounds.first, rightBounds.second - rightBounds.first));
				std::string replacement = "std::pow(" + left + ", " + right + ")";
				text.replace(leftBounds.first, rightBounds.second - leftBounds.first, replacement);
				pos = leftBounds.first + replacement.size();
			}

			return text;
		}

		std::string processSegment(std::string segment, std::vector<std::string>& errors, std::size_t lineNumber)
		{
			auto converted = convertExponent(segment, errors, lineNumber);
			replaceWord(converted, "правда", "true");
			replaceWord(converted, "ложь", "false");
			replaceWord(converted, "и", "&&");
			replaceWord(converted, "или", "||");
			replaceWord(converted, "не", "!");
			return converted;
		}

		std::string translateExpression(const std::string& expression,
		                                 std::vector<std::string>& errors,
		                                 std::size_t lineNumber)
		{
			std::ostringstream result;
			std::string segment;
			bool inString = false;

			for (std::size_t i = 0; i < expression.size(); ++i)
			{
				char c = expression[i];
				if (inString)
				{
					result << c;
					if (c == '"' && (i == 0 || expression[i - 1] != '\\'))
					{
						inString = false;
					}
					continue;
				}

				if (c == '"' && (i == 0 || expression[i - 1] != '\\'))
				{
					if (!segment.empty())
					{
						result << processSegment(segment, errors, lineNumber);
						segment.clear();
					}
					result << c;
					inString = true;
				}
				else
				{
					segment.push_back(c);
				}
			}

			if (inString)
			{
				errors.emplace_back("Строка " + std::to_string(lineNumber) +
				                    ": строковый литерал не закрыт.");
			}

			if (!segment.empty())
			{
				result << processSegment(segment, errors, lineNumber);
			}

			return trimCopy(result.str());
		}

		std::size_t findAssignmentPos(const std::string& text)
		{
			bool inString = false;
			for (std::size_t i = 0; i < text.size(); ++i)
			{
				char c = text[i];
				if (c == '"' && (i == 0 || text[i - 1] != '\\'))
				{
					inString = !inString;
					continue;
				}

				if (inString)
				{
					continue;
				}

				if (c == '=')
				{
					if (i + 1 < text.size() && text[i + 1] == '=')
					{
						++i;
						continue;
					}

					if (i > 0)
					{
						char prev = text[i - 1];
						if (prev == '<' || prev == '>' || prev == '!' || prev == '=')
						{
							continue;
						}
					}

					return i;
				}
			}

			return std::string::npos;
		}

		bool isAssignmentStatement(const std::string& text)
		{
			return findAssignmentPos(text) != std::string::npos;
		}
	} // namespace

	struct TranslationResult
	{
		bool success = false;
		std::string cppCode;
		std::vector<std::string> errors;
	};

	class BearLangTranslator
	{
	public:
		TranslationResult translate(const std::string& script) const;

	private:
		struct Statement
		{
			std::string cppLine;
			bool opensBlock = false;
		};

		std::optional<Statement> parseStatement(const std::string& trimmed,
		                                        std::size_t lineNumber,
		                                        std::vector<std::string>& errors) const;

		std::optional<Statement> parseConditional(const std::string& trimmed,
		                                          const std::string& keyword,
		                                          const std::string& cppKeyword,
		                                          std::size_t lineNumber,
		                                          std::vector<std::string>& errors) const;

		std::optional<Statement> parseWhile(const std::string& trimmed,
		                                    std::size_t lineNumber,
		                                    std::vector<std::string>& errors) const;

		std::optional<Statement> parseFor(const std::string& trimmed,
		                                  std::size_t lineNumber,
		                                  std::vector<std::string>& errors) const;

		Statement parseInput(const std::string& trimmed,
		                     std::size_t lineNumber,
		                     std::vector<std::string>& errors) const;

		Statement parseOutput(const std::string& trimmed,
		                      std::size_t lineNumber,
		                      std::vector<std::string>& errors) const;

		std::optional<Statement> parseVariableDeclaration(const std::string& trimmed,
		                                                  const std::string& typeKeyword,
		                                                  std::size_t lineNumber,
		                                                  std::vector<std::string>& errors) const;

		Statement parseAssignment(const std::string& trimmed,
		                          std::size_t lineNumber,
		                          std::vector<std::string>& errors) const;
	};

	TranslationResult BearLangTranslator::translate(const std::string& script) const
	{
		TranslationResult result;
		std::istringstream input(script);
		std::string line;
		std::ostringstream body;
		std::vector<int> blockStack;
		std::size_t lineNumber = 0;

		while (std::getline(input, line))
		{
			++lineNumber;

			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			std::string withoutComments = stripComments(line);
			std::string trimmed = trimCopy(withoutComments);
			if (trimmed.empty())
			{
				continue;
			}

			int indent = indentLevel(withoutComments);
			while (!blockStack.empty() && blockStack.back() >= indent)
			{
				body << indentation(blockStack.back()) << "}\n";
				blockStack.pop_back();
			}

			auto statement = parseStatement(trimmed, lineNumber, result.errors);
			if (!statement.has_value())
			{
				continue;
			}

			body << indentation(indent) << statement->cppLine << '\n';
			if (statement->opensBlock)
			{
				blockStack.push_back(indent);
			}
		}

		while (!blockStack.empty())
		{
			body << indentation(blockStack.back()) << "}\n";
			blockStack.pop_back();
		}

		if (!result.errors.empty())
		{
			result.success = false;
			return result;
		}

		std::ostringstream cpp;
		cpp << "#include <iostream>\n"
		    << "#include <string>\n"
		    << "#include <cmath>\n"
		    << "\n"
		    << "int main() {\n"
		    << indentation(0) << "std::ios::sync_with_stdio(false);\n"
		    << indentation(0) << "std::cin.tie(nullptr);\n"
		    << body.str()
		    << indentation(0) << "return 0;\n"
		    << "}\n";

		result.success = true;
		result.cppCode = cpp.str();
		return result;
	}

	std::optional<BearLangTranslator::Statement> BearLangTranslator::parseStatement(const std::string& trimmed,
	                                                                                std::size_t lineNumber,
	                                                                                std::vector<std::string>& errors) const
	{
		if (beginsWithWord(trimmed, "иначе если"))
		{
			return parseConditional(trimmed, "иначе если", "else if", lineNumber, errors);
		}

		if (beginsWithWord(trimmed, "если"))
		{
			return parseConditional(trimmed, "если", "if", lineNumber, errors);
		}

		if (beginsWithWord(trimmed, "иначе"))
		{
			return Statement{"else {", true};
		}

		if (beginsWithWord(trimmed, "пока"))
		{
			return parseWhile(trimmed, lineNumber, errors);
		}

		if (beginsWithWord(trimmed, "для"))
		{
			return parseFor(trimmed, lineNumber, errors);
		}

		if (beginsWithWord(trimmed, "ввод"))
		{
			return parseInput(trimmed, lineNumber, errors);
		}

		if (beginsWithWord(trimmed, "вывод"))
		{
			return parseOutput(trimmed, lineNumber, errors);
		}

		for (const auto& [bearType, cppType] : kTypeMap)
		{
			if (beginsWithWord(trimmed, bearType))
			{
				return parseVariableDeclaration(trimmed, bearType, lineNumber, errors);
			}
		}

		if (isAssignmentStatement(trimmed))
		{
			return parseAssignment(trimmed, lineNumber, errors);
		}

		errors.emplace_back("Строка " + std::to_string(lineNumber) + ": не удалось распознать \"" + trimmed + "\".");
		return std::nullopt;
	}

	std::optional<BearLangTranslator::Statement> BearLangTranslator::parseConditional(
	    const std::string& trimmed,
	    const std::string& keyword,
	    const std::string& cppKeyword,
	    std::size_t lineNumber,
	    std::vector<std::string>& errors) const
	{
		std::size_t openPos = trimmed.find('(');
		std::size_t closePos = trimmed.rfind(')');
		if (openPos == std::string::npos || closePos == std::string::npos || closePos <= openPos)
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": ожидаются круглые скобки после \"" + keyword + "\".");
			return std::nullopt;
		}

		std::string inner = trimCopy(trimmed.substr(openPos + 1, closePos - openPos - 1));
		if (inner.empty())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": условие после \"" + keyword + "\" пустое.");
			return std::nullopt;
		}

		std::string expression = translateExpression(inner, errors, lineNumber);
		return Statement{cppKeyword + " (" + expression + ") {", true};
	}

	std::optional<BearLangTranslator::Statement> BearLangTranslator::parseWhile(const std::string& trimmed,
	                                                                            std::size_t lineNumber,
	                                                                            std::vector<std::string>& errors) const
	{
		return parseConditional(trimmed, "пока", "while", lineNumber, errors);
	}

	std::optional<BearLangTranslator::Statement> BearLangTranslator::parseFor(const std::string& trimmed,
	                                                                          std::size_t lineNumber,
	                                                                          std::vector<std::string>& errors) const
	{
		std::size_t openPos = trimmed.find('(');
		std::size_t closePos = trimmed.rfind(')');
		if (openPos == std::string::npos || closePos == std::string::npos || closePos <= openPos)
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": ожидаются круглые скобки после \"для\".");
			return std::nullopt;
		}

		std::string header = trimCopy(trimmed.substr(openPos + 1, closePos - openPos - 1));
		std::istringstream stream(header);
		std::string typeToken;
		std::string variable;
		if (!(stream >> typeToken >> variable))
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": формат цикла \"для\" — для (тип имя от a до b).");
			return std::nullopt;
		}

		auto typeIt = kTypeMap.find(typeToken);
		if (typeIt == kTypeMap.end())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": неизвестный тип \"" + typeToken + "\" в цикле.");
			return std::nullopt;
		}

		std::string remainder;
		std::getline(stream, remainder);
		remainder = trimCopy(remainder);
		if (!beginsWithWord(remainder, "от"))
		{
			errors.emplace_back(
			    "Строка " + std::to_string(lineNumber) +
			    ": после имени переменной в цикле \"для\" ожидается слово \"от\".");
			return std::nullopt;
		}

		remainder = trimCopy(remainder.substr(std::string("от").size()));
		std::size_t posDo = remainder.find("до");
		if (posDo == std::string::npos)
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": слово \"до\" обязательно для цикла \"для\".");
			return std::nullopt;
		}

		std::string startExpr = trimCopy(remainder.substr(0, posDo));
		std::string endExpr = trimCopy(remainder.substr(posDo + std::string("до").size()));
		if (startExpr.empty() || endExpr.empty())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": границы цикла \"для\" не могут быть пустыми.");
			return std::nullopt;
		}

		std::string startCpp = translateExpression(startExpr, errors, lineNumber);
		std::string endCpp = translateExpression(endExpr, errors, lineNumber);

		std::ostringstream cpp;
		cpp << "for (" << typeIt->second << " " << variable << " = " << startCpp << "; " << variable << " <= "
		    << endCpp << "; ++" << variable << ") {";
		return Statement{cpp.str(), true};
	}

	BearLangTranslator::Statement BearLangTranslator::parseInput(const std::string& trimmed,
	                                                             std::size_t lineNumber,
	                                                             std::vector<std::string>& errors) const
	{
		std::string remainder = trimCopy(trimmed.substr(std::string("ввод").size()));
		if (remainder.empty())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) + ": команда \"ввод\" требует имя переменной.");
			return Statement{"// ошибка ввода", false};
		}

		std::istringstream stream(remainder);
		std::vector<std::string> names;
		std::string token;
		while (stream >> token)
		{
			names.push_back(token);
		}

		std::ostringstream cpp;
		cpp << "std::cin";
		for (const auto& name : names)
		{
			cpp << " >> " << name;
		}
		cpp << ";";

		return Statement{cpp.str(), false};
	}

	BearLangTranslator::Statement BearLangTranslator::parseOutput(const std::string& trimmed,
	                                                              std::size_t lineNumber,
	                                                              std::vector<std::string>& errors) const
	{
		std::string remainder = trimCopy(trimmed.substr(std::string("вывод").size()));
		if (remainder.empty())
		{
			return Statement{"std::cout << std::endl;", false};
		}

		std::string expression = translateExpression(remainder, errors, lineNumber);
		std::string cpp = "std::cout << " + expression + " << std::endl;";
		return Statement{cpp, false};
	}

	std::optional<BearLangTranslator::Statement> BearLangTranslator::parseVariableDeclaration(
	    const std::string& trimmed,
	    const std::string& typeKeyword,
	    std::size_t lineNumber,
	    std::vector<std::string>& errors) const
	{
		auto typeIt = kTypeMap.find(typeKeyword);
		if (typeIt == kTypeMap.end())
		{
			return std::nullopt;
		}

		std::string remainder = trimCopy(trimmed.substr(typeKeyword.size()));
		if (remainder.empty())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": после типа должна идти переменная.");
			return std::nullopt;
		}

		std::size_t assignPos = findAssignmentPos(remainder);
		if (assignPos == std::string::npos)
		{
			return Statement{typeIt->second + " " + remainder + ";", false};
		}

		std::string varName = trimCopy(remainder.substr(0, assignPos));
		std::string value = trimCopy(remainder.substr(assignPos + 1));
		if (varName.empty() || value.empty())
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) +
			                    ": объявление переменной должно иметь вид \"тип имя = значение\".");
			return std::nullopt;
		}

		std::string expression = translateExpression(value, errors, lineNumber);
		return Statement{typeIt->second + " " + varName + " = " + expression + ";", false};
	}

	BearLangTranslator::Statement BearLangTranslator::parseAssignment(const std::string& trimmed,
	                                                                  std::size_t lineNumber,
	                                                                  std::vector<std::string>& errors) const
	{
		std::size_t pos = findAssignmentPos(trimmed);
		if (pos == std::string::npos)
		{
			errors.emplace_back("Строка " + std::to_string(lineNumber) + ": не удалось разобрать присваивание.");
			return Statement{"// ошибка присваивания", false};
		}

		std::string target = trimCopy(trimmed.substr(0, pos));
		std::string value = trimCopy(trimmed.substr(pos + 1));
		std::string expression = translateExpression(value, errors, lineNumber);
		return Statement{target + " = " + expression + ";", false};
	}

	class BearLangCompiler
	{
	public:
		bool compileAndRun(const std::string& cppCode) const;

	private:
		std::filesystem::path createTempPath(const std::string& extension) const;
	};

	std::filesystem::path BearLangCompiler::createTempPath(const std::string& extension) const
	{
		namespace fs = std::filesystem;
		auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		std::ostringstream name;
		name << "bearlang_program_" << timestamp << extension;
		return fs::temp_directory_path() / name.str();
	}

	bool BearLangCompiler::compileAndRun(const std::string& cppCode) const
	{
		namespace fs = std::filesystem;

		fs::path cppPath = createTempPath(".cpp");
#ifdef _WIN32
		fs::path binPath = createTempPath(".exe");
#else
		fs::path binPath = createTempPath("");
#endif

		{
			std::ofstream out(cppPath);
			if (!out)
			{
				std::cerr << "Не удалось создать временный файл: " << cppPath << std::endl;
				return false;
			}
			out << cppCode;
		}

		std::ostringstream compileCommand;
		compileCommand << "g++ -std=c++20 -O2 -Wall -Wextra -pedantic \"" << cppPath.string() << "\" -o \""
		               << binPath.string() << "\"";

		std::cout << "\nКомпиляция C++ кода..." << std::endl;
		int compileCode = std::system(compileCommand.str().c_str());
		if (compileCode != 0)
		{
			std::cerr << "Компиляция завершилась с кодом " << compileCode << ". Проверьте сгенерированный код."
			          << std::endl;
			std::error_code ec;
			fs::remove(cppPath, ec);
			fs::remove(binPath, ec);
			return false;
		}

		std::cout << "----- Запуск программы -----" << std::endl;
		std::string runCommand = "\"" + binPath.string() + "\"";
		int runCode = std::system(runCommand.c_str());
		if (runCode != 0)
		{
			std::cerr << "Программа завершилась с кодом " << runCode << "." << std::endl;
		}
		else
		{
			std::cout << "----- Программа завершена -----" << std::endl;
		}

		std::error_code ec;
		fs::remove(cppPath, ec);
		fs::remove(binPath, ec);

		return runCode == 0;
	}

	void TutorApp::run() const
	{
		std::cout << "Добро пожаловать в BearLang Tutor!\n"
		             "Здесь вы можете писать программы на языке BearLang,\n"
		             "видеть перевод в C++ и немедленно запускать результат.\n";

		while (true)
		{
			showMenu();
			std::cout << "> ";
			std::string choice;
			if (!std::getline(std::cin, choice))
			{
				std::cout << "\nДо новых встреч!\n";
				return;
			}

			std::string normalized = trim(choice);
			if (normalized == "1")
			{
				handleProgramInput();
			}
			else if (normalized == "2")
			{
				showSampleProgram();
			}
			else if (normalized == "3")
			{
				std::cout << "До новых встреч! Продолжайте исследовать BearLang.\n";
				break;
			}
			else if (!normalized.empty())
			{
				std::cout << "Неизвестный выбор \"" << normalized << "\". Пожалуйста, попробуйте снова.\n";
			}
		}
	}

	void TutorApp::showMenu() const
	{
		std::cout << "\nЧто вы хотите сделать?\n"
		             "  1. Написать и выполнить программу BearLang\n"
		             "  2. Посмотреть пример программы\n"
		             "  3. Выйти\n";
	}

	void TutorApp::handleProgramInput() const
	{
		std::cout << "\nВводите программу BearLang построчно. Когда закончите, напишите \"конец\" на отдельной строке.\n";

		std::ostringstream script;
		std::string line;
		while (true)
		{
			std::cout << "│ ";
			if (!std::getline(std::cin, line))
			{
				std::cout << "\nВвод завершён.\n";
				return;
			}

			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			if (trim(line) == "конец")
			{
				break;
			}

			script << line << '\n';
		}

		std::string program = trim(script.str());
		if (program.empty())
		{
			std::cout << "Пустая программа. Попробуйте снова!\n";
			return;
		}

		BearLangTranslator translator;
		auto translation = translator.translate(program);
		if (!translation.success)
		{
			std::cout << "Не удалось перевести программу. Исправьте ошибки:\n";
			for (const auto& error : translation.errors)
			{
				std::cout << " - " << error << '\n';
			}
			return;
		}

		std::cout << "\n----- Сгенерированный C++ -----\n"
		          << translation.cppCode
		          << "-------------------------------\n";

		BearLangCompiler compiler;
		if (!compiler.compileAndRun(translation.cppCode))
		{
			std::cout << "Запуск не удался. Проверьте сообщения об ошибках выше.\n";
		}
	}

	void TutorApp::showSampleProgram() const
	{
		std::cout << "\nПример программы BearLang:\n"
		             "целое число = 5\n"
		             "целое попытка = 0\n"
		             "пока (попытка < число)\n"
		             "\tвывод \"Привет, BearLang!\"\n"
		             "\tпопытка = попытка + 1\n"
		             "если (число >= 5)\n"
		             "\tвывод \"Число достаточно большое\"\n"
		             "иначе\n"
		             "\tвывод \"Нужно побольше\"\n";
	}

	std::string TutorApp::trim(const std::string& value)
	{
		return trimCopy(value);
	}
} // namespace bear

int main()
{
	bear::TutorApp app;
	app.run();
	return 0;
}