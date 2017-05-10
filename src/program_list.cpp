#include "program_list.hpp"

ProgramList::ProgramList(const std::set<std::string> & paths_, const std::set<std::string> & substrings)
	: paths(paths_), title_substrings(substrings) {}

bool ProgramList::satisfies(const ForeignWindow & wnd) {
	std::lock_guard<std::mutex> lock(mutex);
	if (paths.count(wnd.get_program_path())) return true;
	std::string title = wnd.get_window_title();
	for (const std::string& substr: title_substrings)
		if (title.find(substr) != std::string::npos) return true;
	return false;
}

std::set<std::string> ProgramList::get_paths() { return paths; }
std::set<std::string> ProgramList::get_substrings() { return title_substrings; }
