#include "foreign_window.hpp"
#include "procrastinase.hpp"
#include <string>
#include <set>
#include <list>

class ProgramList {
public:
	bool satisfies(const ForeignWindow&);
private:
	std::set<std::string> paths;
	std::list<std::string> title_substrings;
};
