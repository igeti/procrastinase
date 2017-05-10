#include "program_list.hpp"

ProgramList::ProgramList() {}

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

std::set<std::string> ProgramList::get_paths() {
	std::lock_guard<std::mutex> lock(mutex);
	return paths;
}

std::set<std::string> ProgramList::get_substrings() {
	std::lock_guard<std::mutex> lock(mutex);
	return title_substrings;
}

void ProgramList::add_path(const std::string & path) {
	std::lock_guard<std::mutex> lock(mutex);
	paths.insert(path);
}

void ProgramList::remove_path(const std::string & path) {
	std::lock_guard<std::mutex> lock(mutex);
	paths.erase(path);
}

void ProgramList::add_substring(const std::string & substring) {
	std::lock_guard<std::mutex> lock(mutex);
	title_substrings.insert(substring);
}

void ProgramList::remove_substring(const std::string & substring) {
	std::lock_guard<std::mutex> lock(mutex);
	title_substrings.erase(substring);
}
