#include "statistics.hpp"
#include <iostream>

void Statistics::add_program_usage_interval(std::string program, std::string title, std::chrono::steady_clock::duration dur, ProgramRole role) {
	std::cout \
		<< "Program:\t" << program << std::endl
		<< "Title:\t" << title << std::endl
		<< "For:\t"
			<< std::chrono::duration_cast<std::chrono::duration<double>>(dur).count()
			<< " seconds" << std::endl
		<< "Role:\t" << (int)role << std::endl;
}
