#include "window_watcher.hpp"

int main() {
	Statistics stat;
	WindowWatcher ww(stat);
	ww.run();
	return 0;
}
