#include "window_watcher.hpp"

int main() {
	std::set<std::string> whitelist = {"/usr/bin/stterm"};
	WindowWatcher ww;
	ww.set_whitelist(whitelist);
	ww.run();
	return 0;
}
