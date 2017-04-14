#include "foreign_window.hpp"
#include "queue.hpp"
#include "timer.hpp"

struct WindowWatcherImpl;

struct WindowEvent {
	enum class WindowEventType {
		new_active,
		new_title,
		alarm_timer,
		kill_timer
	} type;
	ForeignWindow wnd;
	std::string title;
	WindowEvent(WindowEventType t, ForeignWindow && w)
		: type(t), wnd(std::move(w)) {}
	WindowEvent(std::string t, ForeignWindow && w)
		: type(WindowEventType::new_title), wnd(std::move(w)), title(t) {}
};

class WindowWatcher {
public:
	WindowWatcher();
	void run();
	void window_title_changed(const ForeignWindow&);
	void active_window_changed(const ForeignWindow&);
private:
	double work_to_play;
	int play_credit_left_ms; // QTimer accepts int
	// TODO: alarm sound object & program list object
	thr_queue<WindowEvent> messages;
	thr_timer<void (WindowWatcher::*)(const ForeignWindow&)> tmr;
	std::unique_ptr<WindowWatcherImpl> impl;
};
