#include "window_watcher.hpp"
#include "private_xcb.hpp"
#include <xcb/xcb.h>
#include <memory>
#include <stdexcept>

// static member storage
xcb_atom_t XCB_atoms::NET_ACTIVE_WINDOW;
xcb_atom_t XCB_atoms::NET_WM_NAME;
xcb_atom_t XCB_atoms::UTF8_STRING;
xcb_atom_t XCB_atoms::NET_WM_PID;

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
			throw("xcb_intern_atom returned error");
		}
		return atom_reply->atom;
	}

	void active_window_thread(thr_queue<WindowEvent> & q) {
		// get the root window
		xcb_screen_t * screen = xcb_setup_roots_iterator(xcb_get_setup(conn.get())).data;
		const uint32_t select_input_val[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
		// "open" root window and set event mask
		xcb_void_cookie_t root_cookie = xcb_change_window_attributes_checked(conn.get(), screen->root, XCB_CW_EVENT_MASK, select_input_val);
		xcb_generic_error_t *error = xcb_request_check(conn.get(), root_cookie);
		if (error != nullptr) {
			throw std::runtime_error("Couldn't open root window"); // FIXME: this is impossible to catch from main thread
		}
		// get events corresponding to PropertyNotify event, atom _NET_ACTIVE_WINDOW
		std::unique_ptr<xcb_generic_event_t,decltype(&free)> event {nullptr,&free};
//		while
		// push them to the queue
	}
	void window_title_thread(thr_queue<WindowEvent> & q, const ForeignWindow & w) {
		// FIXME: how to cancel this thread safely?
		// FIXME: it seems that I'll need to create them for each active window
		// open the window
		// set event mask to PropertyNotify, atom _NET_WM_NAME
		// push events to the queue
	}
	WindowWatcherImpl() :conn{nullptr,&xcb_disconnect} {}
};

WindowWatcher::WindowWatcher() {
	impl->conn.reset(xcb_connect(nullptr, nullptr));
	if (!impl->conn) throw std::runtime_error("xcb_connect returned error");
	XCB_atoms::NET_WM_NAME = impl->get_atom("_NET_WM_NAME");
	XCB_atoms::NET_ACTIVE_WINDOW = impl->get_atom("_NET_ACTIVE_WINDOW");
	XCB_atoms::UTF8_STRING = impl->get_atom("UTF8_STRING");
	XCB_atoms::NET_WM_PID = impl->get_atom("_NET_WM_PID");
}
