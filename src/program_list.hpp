#include "foreign_window.hpp"
#include "procrastinase.hpp"
#include <string>
#include <set>
#include <mutex>

class ProgramList {
public:
	ProgramList(const std::set<std::string> &, const std::set<std::string>&);
	bool satisfies(const ForeignWindow&);
	const std::set<std::string> & get_paths();
	const std::set<std::string> & get_substrings();
	void add_path(const std::string &);
	void remove_path(const std::string &);
	void add_substring(const std::string &);
	void remove_substring(const std::string &);
private:
	std::set<std::string> paths;
	std::set<std::string> title_substrings;
	std::mutex mutex;
};
