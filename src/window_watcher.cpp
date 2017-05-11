#include "window_watcher.hpp"

// the business logic(TM) method
void WindowWatcher::handle_event(const WindowEvent & ev) {
	if (ev.type == WindowEvent::Type::new_active
	|| ev.type == WindowEvent::Type::new_title
	|| ev.type == WindowEvent::Type::no_active) { // window activity
		std::chrono::steady_clock::time_point now {
			std::chrono::steady_clock::now()
		};
		std::chrono::steady_clock::duration dur{now - last_event};
		last_event = now;
		stat.add_program_usage_interval(
			last_program,
			last_title,
			dur,
			last_state
		);
	}
	switch (ev.type) {
	case WindowEvent::Type::new_active:
	case WindowEvent::Type::new_title:
		last_program = ev.wnd.get_program_path();
		last_title = ev.wnd.get_window_title();
		last_state = ProgramRole::grey; // TODO *-list logic
		break;
	case WindowEvent::Type::no_active:
		last_program = last_title = ""; // yay, high-level strings!
		last_state = ProgramRole::grey; // no active program means timer is inactive too
		break;
	case WindowEvent::Type::alarm_timer:
		// TODO: sound an alarm, arm the kill timer
	case WindowEvent::Type::kill_timer:
		// TODO: kill the app
		throw "Unimplemented";
	}
}
