#include "foreign_window.hpp"

struct WindowWatcherImpl;

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
	std::unique_ptr<WindowWatcherImpl> impl;
};
