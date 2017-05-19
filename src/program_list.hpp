#pragma once
#include "foreign_window.hpp"
#include "procrastinase.hpp"
#include <string>
#include <unordered_set>
#include <mutex>

class ProgramList {
public:
	ProgramList();
	ProgramList(const std::unordered_set<std::string> &, const std::unordered_set<std::string>&);
	bool satisfies(const ForeignWindow&);
	std::unordered_set<std::string> get_paths();
	std::unordered_set<std::string> get_substrings();
	void add_path(const std::string &);
	void remove_path(const std::string &);
	void add_substring(const std::string &);
	void remove_substring(const std::string &);
private:
	std::unordered_set<std::string> paths;
	std::unordered_set<std::string> title_substrings;
	std::mutex mutex;
};
