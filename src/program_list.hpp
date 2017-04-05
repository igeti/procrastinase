#include "foreign_window.hpp"
#include "procrastinase.hpp"
#include <string>
#include <set>
#include <vector>

class ProgramList {
public:
	const ProgramRole type;
	virtual bool satisfies(const ForeignWindow&) = 0;
	virtual ~ProgramList();
};

class ProgramPathSet {
public:
	ProgramPathSet(const std::set<std::string>&);
	bool satisfies(const ForeignWindow&);
private:
	std::set<std::string> paths;
};

class WindowTitleSubstringList {
public:
	WindowTitleSubstringList(const std::vector<std::string>&);
	bool satisfies(const ForeignWindow&);
private:
	std::vector<std::string> title_substrings;
};
