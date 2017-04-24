#include "foreign_window.hpp"
#include "queue.hpp"
#include "timer.hpp"
#include "statistics.hpp"
#include <set> // FIXME: for now

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
	void handle_event(const WindowEvent &);
	void set_whitelist(const std::set<std::string>&); // FIXME: for now
	~WindowWatcher();
private:
	Statistics & stat;
	std::chrono::steady_clock::time_point last_event;
	std::string last_program, last_title;
	ProgramRole last_state;
	// TODO: alarm sound object
	std::set<std::string> whitelist; // FIXME: for now
	thr_queue<WindowEvent> messages;
	thr_timer tmr;
	std::unique_ptr<WindowWatcherImpl> impl;
};
