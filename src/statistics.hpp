#include <string>
#include <chrono>
#include "procrastinase.hpp"

class Statistics {
public:
	void add_program_usage_interval(std::string, std::string, std::chrono::steady_clock::duration, ProgramRole);
};
