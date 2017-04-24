#include "foreign_window.hpp"
#include "procrastinase.hpp"
#include <string>
#include <set>
#include <list>

class ProgramList {
public:
	const ProgramRole type;
	virtual bool satisfies(const ForeignWindow&) = 0;
	virtual ~ProgramList();
};

class ProgramPathSet : public ProgramList {
public:
	ProgramPathSet(const std::set<std::string>&);
	bool satisfies(const ForeignWindow&) override;
private:
	std::set<std::string> paths;
};

class WindowTitleSubstringList : public ProgramList {
public:
	WindowTitleSubstringList(const std::list<std::string>&);
	bool satisfies(const ForeignWindow&) override;
private:
	std::list<std::string> title_substrings;
};
