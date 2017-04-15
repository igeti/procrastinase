#include <memory>
#include <string>

struct ForeignWindowImpl;
struct WindowWatcherImpl;

class ForeignWindow {
// only WW has enough access to system guts to access window IDs
friend class WindowWatcher;
friend struct WindowWatcherImpl;
public:
	void kill();
	std::string get_window_title();
	std::string get_program_path();
private:
	std::unique_ptr<ForeignWindowImpl> impl;
	// user doesn't know about window system and can't know their IDs
	ForeignWindow(ForeignWindowImpl);
};
