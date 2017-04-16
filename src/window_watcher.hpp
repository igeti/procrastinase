#include "foreign_window.hpp"
#include "queue.hpp"
#include "timer.hpp"
#include <set> // FIXME: for now

struct WindowWatcherImpl;

struct WindowEvent {
	enum class Type {
		new_active,
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
	WindowWatcher();
	void run();
	void window_title_changed(const ForeignWindow&);
	void active_window_changed(const ForeignWindow&);
	void set_whitelist(const std::set<std::string>&);
	~WindowWatcher();
private:
	double work_to_play;
	std::chrono::steady_clock::duration play_credit_left_ms;
	// TODO: alarm sound object & program list object
	std::set<std::string> whitelist; // FIXME: for now
	thr_queue<WindowEvent> messages;
	thr_timer tmr;
	std::unique_ptr<WindowWatcherImpl> impl;
};
