#include "window_watcher.hpp"
#include "private_xcb.hpp"

// static member storage
xcb_atom_t XCB::NET_ACTIVE_WINDOW;
xcb_atom_t XCB::NET_WM_NAME;
xcb_atom_t XCB::UTF8_STRING;
xcb_atom_t XCB::NET_WM_PID;

struct WindowWatcherImpl {
	std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> conn;

	xcb_atom_t get_atom(std::string atom_name) {
		xcb_intern_atom_cookie_t atom_cookie;
		std::unique_ptr<xcb_intern_atom_reply_t,decltype(&free)> atom_reply{nullptr,&free};
		xcb_generic_error_t *err;

		atom_cookie = xcb_intern_atom(conn.get(), 0, atom_name.length(), atom_name.c_str());
		atom_reply.reset(xcb_intern_atom_reply(conn.get(), atom_cookie, &err));
		if (!atom_reply) {
			if (err) free(err);
			throw std::runtime_error("xcb_intern_atom returned error");
		}
		return atom_reply->atom;
	}

	void active_window_thread(thr_queue<WindowEvent> & q) try {
		// get the root window
		xcb_screen_t * screen = xcb_setup_roots_iterator(xcb_get_setup(conn.get())).data;
		const uint32_t select_input_val[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
		// "open" root window and set event mask
		xcb_void_cookie_t root_cookie = xcb_change_window_attributes_checked(conn.get(), screen->root, XCB_CW_EVENT_MASK, select_input_val);
		ForeignWindow root({conn.get(), screen->root});
		xcb_generic_error_t *error = xcb_request_check(conn.get(), root_cookie);
		if (error != nullptr) {
			free(error); // FIXME: XCB doesn't tell me if I should
			throw std::runtime_error("Couldn't open root window");
		}
		// get events corresponding to PropertyNotify event, atom _NET_ACTIVE_WINDOW
		std::unique_ptr<xcb_generic_event_t,decltype(&free)> event {nullptr,&free};
		for (event.reset(xcb_wait_for_event(conn.get())); event; event.reset(xcb_wait_for_event(conn.get()))) {
			if (event->response_type == XCB_PROPERTY_NOTIFY) {
				xcb_property_notify_event_t * pne = reinterpret_cast<xcb_property_notify_event_t*>(event.get());
				if (pne->atom == XCB::NET_ACTIVE_WINDOW) {
					// push them to the queue
					xcb_window_t new_window = root.impl->get_property<xcb_window_t>(
						XCB::NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW
					);
					if (new_window)
						q.push({WindowEvent::Type::new_active, ForeignWindowImpl{conn.get(), new_window}});
					else
						q.push({WindowEvent::Type::no_active, ForeignWindowImpl{conn.get(), screen->root/*whatever*/}});
				}
			}
		}
	} catch(...) {
		// there is noone to catch errors outside this function because it's in a separate thread
		q.finish(); // main thread will be notified and should investigate
		return;
	}

	void window_title_thread(thr_queue<WindowEvent> & q, const ForeignWindow & w) {
		// to cancel this thread safely, I need a flag to check on every message and return if not needed anymore
		// FIXME: it seems that I'll need to create them for each active window
		// open the window
		// set event mask to PropertyNotify, atom _NET_WM_NAME
		// push events to the queue
	}

	WindowWatcherImpl() :conn{nullptr,&xcb_disconnect} {}
};

WindowWatcher::WindowWatcher(Statistics & stat_)
	: stat(stat_), last_event(std::chrono::steady_clock::now()),
	last_state(ProgramRole::grey), impl(new WindowWatcherImpl) {
	impl->conn.reset(xcb_connect(nullptr, nullptr));
	if (!impl->conn) throw std::runtime_error("xcb_connect returned error");
	XCB::NET_WM_NAME = impl->get_atom("_NET_WM_NAME");
	XCB::NET_ACTIVE_WINDOW = impl->get_atom("_NET_ACTIVE_WINDOW");
	XCB::UTF8_STRING = impl->get_atom("UTF8_STRING");
	XCB::NET_WM_PID = impl->get_atom("_NET_WM_PID");
}

WindowWatcher::~WindowWatcher() = default;

void WindowWatcher::run() {
	auto window_thread = std::thread(&WindowWatcherImpl::active_window_thread, impl.get(), std::ref(messages));
	window_thread.detach();
	try {
		for (;;) {
			WindowEvent we{std::move(messages.pop())};
			handle_event(we);
		}
	} catch (thr_finished& f) {
		throw std::runtime_error("XCB thread found error");
		// TODO: When we implement an actual termination command for the app, there'll be a flag about that
		return;
	}
}