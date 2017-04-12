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
	// ewww, why did I have to wrap this is std::function?
	std::unique_ptr<xcb_connection_t, std::function<decltype(xcb_disconnect)>> conn;

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
