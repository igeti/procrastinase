#include "window_watcher.hpp"

int main() {
	std::set<std::string> whitelist = {"/usr/bin/stterm"};

	Statistics stat;

	WindowWatcher ww(stat);
//	ww.set_whitelist(whitelist);

	ww.run();
	return 0;
}
