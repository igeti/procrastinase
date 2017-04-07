#include "foreign_window.hpp"
#include "private_xcb.h"
#include <stdexcept>

const size_t title_length = 4096;

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl) {
	*impl = impl_; /* you know what you're doing */
}

std::string ForeignWindow::get_window_title() {
	xcb_generic_error_t *err = NULL;
	char title[title_length];
	xcb_get_property_cookie_t title_cookie = xcb_get_property(
		impl->conn, 0, impl->wid, XCB_atoms::NET_WM_NAME, XCB_atoms::UTF8_STRING,
		0, title_length/sizeof(uint32_t) /* WTF! */
	);
	xcb_get_property_reply_t* title_reply = xcb_get_property_reply(impl->conn, title_cookie, &err);
	// ...
	if (title_reply) free(title_reply);
	if (err) {
		free(err);
		throw std::runtime_error("xcb_get_property returned error");
	}
	return std::string(title);
}
