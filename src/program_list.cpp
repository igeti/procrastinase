#include "program_list.hpp"

ProgramList::satisfies(const ForeignWindow & wnd) {
	std::lock_guard<std::mutex> lock(mutex);
	if (paths.count(wnd.get_program_path())) return true;
	std::string title = wnd.get_window_title();
	for (const std::string& substr: title_substrings)
		if (title.find(substr) != std::string::npos) return true;
	return false;
}
		
