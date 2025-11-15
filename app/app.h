#pragma once

#include <string>

namespace bear
{
	class TutorApp
	{
	public:
		void run() const;

	private:
		void showMenu() const;
		void handleProgramInput() const;
		void showSampleProgram() const;
		static std::string trim(const std::string& value);
	};
}
