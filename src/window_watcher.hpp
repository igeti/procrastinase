#include <memory>
#include "foreign_window.hpp"

class WindowWatcher {
public:
	void run();
	void onWindowTitleChange(ForeignWindow&);
	void onActiveWindowChange(ForeignWindow&);
private:
	double work_to_play;
	time_t play_credit_left;
	// TODO: alarm sound object & program list object
	std::unique_ptr<WindowWatcherImpl> impl;
}
