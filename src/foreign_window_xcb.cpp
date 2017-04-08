#define _POSIX_C_SOURCE 200809L // readlink
#include "foreign_window.hpp"
#include "private_xcb.hpp"
#include <stdexcept>
#include <memory>
#include <sys/types.h> // pid_t
#include <cmath> // ceil
#include <stdlib.h> // realpath

const size_t title_path_length = 4096; // VFS has troubles coping with >256-character paths, anyway

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl) {
	*impl = impl_; /* you know what you're doing */
}

std::string ForeignWindow::get_window_title() {
	using std::unique_ptr;
	using std::runtime_error;

	xcb_generic_error_t *err = nullptr; // can't use unique_ptr here because get_property_reply overwrites pointer value

	xcb_get_property_cookie_t title_cookie = xcb_get_property(
		impl->conn, 0, impl->wid, XCB_atoms::NET_WM_NAME, XCB_atoms::UTF8_STRING,
		0, title_path_length/sizeof(uint32_t) /* FIXME: what happens if I pass 0? */
	);

	unique_ptr<xcb_get_property_reply_t> title_reply {xcb_get_property_reply(impl->conn, title_cookie, &err)};
	if (!title_reply) {
		if (err) {
			free(err);
			throw runtime_error("xcb_get_property returned error");
		} else throw runtime_error("xcb_get_property returned nullptr and no error");
	}

	// XXX: property_value shouldn't be freed: it's part of the title_reply memory block
	return std::string(reinterpret_cast<char*>(xcb_get_property_value(title_reply.get())));
}

static std::string executable_path(pid_t pid) {
	std::string link {"/proc/"};
	link += pid;
	link += "/exe";
	std::unique_ptr<char,decltype(&std::free)> path{realpath(link.c_str(),nullptr),&std::free};
	if (!path) throw std::runtime_error("realpath returned error");
	return std::string(path.get());
}

std::string ForeignWindow::get_program_path() {
	using std::unique_ptr;
	using std::runtime_error;

	xcb_generic_error_t *err = nullptr; // can't use unique_ptr here because get_property_reply overwrites pointer value

	xcb_get_property_cookie_t pid_cookie = xcb_get_property(
		impl->conn, 0, impl->wid, XCB_atoms::NET_WM_PID, XCB_ATOM_CARDINAL,
		0, std::ceil(float(sizeof(pid_t))/sizeof(uint32_t)) /* FIXME: how much is a CARDINAL? */
	);
	unique_ptr<xcb_get_property_reply_t> pid_reply {xcb_get_property_reply(impl->conn, pid_cookie, &err)};
	if (!pid_reply) {
		if (err) {
			free(err);
			throw runtime_error("xcb_get_property returned error");
		} else throw runtime_error("xcb_get_property returned nullptr and no error");
	}
	return executable_path(*reinterpret_cast<pid_t*>(xcb_get_property_value(pid_reply.get())));
}
