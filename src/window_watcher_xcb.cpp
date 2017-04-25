#include "window_watcher.hpp"
#include "private_xcb.hpp"

// static member storage
xcb_atom_t XCB::NET_ACTIVE_WINDOW;
xcb_atom_t XCB::NET_WM_NAME;
xcb_atom_t XCB::UTF8_STRING;
xcb_atom_t XCB::NET_WM_PID;

const size_t title_length = 4096;

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

	xcb_window_t currently_watched;

	void set_window_events(xcb_window_t wid, uint32_t value) {
		const uint32_t select_input_val[] = { value };
		xcb_void_cookie_t event_cookie = xcb_change_window_attributes_checked(conn.get(), wid, XCB_CW_EVENT_MASK, select_input_val);
		xcb_generic_error_t *error = xcb_request_check(conn.get(), event_cookie);
		if (error != nullptr) {
			free(error); // FIXME: XCB doesn't tell me if I should
			throw std::runtime_error("Couldn't change window event mask");
		}
	}

	void watch_window_title(const ForeignWindow & wnd) {
		currently_watched = wnd.impl->wid;
		set_window_events(wnd.impl->wid, XCB_EVENT_MASK_PROPERTY_CHANGE);
	}

	void unwatch_window(const ForeignWindow & wnd) {
		currently_watched = 0;
		set_window_events(wnd.impl->wid, XCB_EVENT_MASK_NO_EVENT);
	}

	void active_window_thread(thr_queue<WindowEvent> & q) try {
		// get the root window
		xcb_screen_t * screen = xcb_setup_roots_iterator(xcb_get_setup(conn.get())).data;
		ForeignWindowImpl root{conn.get(), screen->root};
		// "open" root window and set event mask
		set_window_events(screen->root, XCB_EVENT_MASK_PROPERTY_CHANGE);
		// get events corresponding to PropertyNotify event, atom _NET_ACTIVE_WINDOW
		std::unique_ptr<xcb_generic_event_t,decltype(&free)> event {nullptr,&free};
		for (event.reset(xcb_wait_for_event(conn.get())); event; event.reset(xcb_wait_for_event(conn.get()))) {
			if (event->response_type == XCB_PROPERTY_NOTIFY) {
				xcb_property_notify_event_t * pne = reinterpret_cast<xcb_property_notify_event_t*>(event.get());
				if (pne->atom == XCB::NET_ACTIVE_WINDOW && pne->window == screen->root) {
					// push them to the queue
					xcb_window_t new_window = root.get_property<xcb_window_t>(
						XCB::NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW
					);
					if (new_window)
						q.push({WindowEvent::Type::new_active, ForeignWindowImpl{conn.get(), new_window}});
					else
						q.push({WindowEvent::Type::no_active, ForeignWindowImpl{conn.get(), screen->root/*whatever*/}});
				} else if (pne->atom == XCB::NET_WM_NAME && pne->window == currently_watched) {
					ForeignWindow w{{conn.get(),pne->window}};
					q.push({w.impl->get_property(XCB::NET_WM_NAME,title_length),std::move(w)});
				}
			}
		}
	} catch(...) {
		// there is noone to catch errors outside this function because it's in a separate thread
		q.finish(); // main thread will be notified and should investigate
		return;
	}

	WindowWatcherImpl() :conn{nullptr,&xcb_disconnect}, currently_watched{0} /* 0 seems to be always invalid */ {}
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
