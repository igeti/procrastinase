#pragma once
#include "foreign_window.hpp"
#include "queue.hpp"
#include "timer.hpp"
#include "statistics.hpp"
#include "program_list.hpp"

struct WindowWatcherImpl;

struct WindowEvent {
	enum class Type {
		new_active,
		no_active,
		new_title,
		alarm_timer,
		kill_timer
	} type;
	ForeignWindow wnd;
	std::string title;
	WindowEvent(Type t, ForeignWindow && w)
		: type(t), wnd(std::move(w)) {}
	WindowEvent(std::string t, ForeignWindow && w)
		: type(Type::new_title), wnd(std::move(w)), title(t) {}
};

class WindowWatcher {
public:
	WindowWatcher(Statistics &);
	void run();
	~WindowWatcher();
private:
	void handle_event(const WindowEvent &);
	Statistics & stat;
	std::chrono::steady_clock::time_point last_event;
	std::string last_program, last_title;
	ProgramRole last_state;
	ProgramList whitelist, greylist, blacklist;
	ProgramRole default_action;
	// TODO: alarm sound object
	thr_queue<WindowEvent> messages;
	thr_timer tmr;
	std::unique_ptr<WindowWatcherImpl> impl;
};
