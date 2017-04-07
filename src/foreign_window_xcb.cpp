#include "foreign_window.hpp"
#include "private_xcb.h"
#include <stdexcept>
#include <memory>

const size_t title_length = 4096; // FIXME: what happens if I pass 0?

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl) {
	*impl = impl_; /* you know what you're doing */
}

std::string ForeignWindow::get_window_title() {
	using std::unique_ptr;
	using std::runtime_error;

	xcb_generic_error_t *err = nullptr; // can't use unique_ptr here because get_property_reply overwrites pointer value

	xcb_get_property_cookie_t title_cookie = xcb_get_property(
		impl->conn, 0, impl->wid, XCB_atoms::NET_WM_NAME, XCB_atoms::UTF8_STRING,
		0, title_length/sizeof(uint32_t) /* WTF?! */
	);

	unique_ptr<xcb_get_property_reply_t> title_reply {xcb_get_property_reply(impl->conn, title_cookie, &err)};
	if (!title_reply) {
		if (err) {
			free(err);
			throw runtime_error("xcb_get_property returned error");
		} else throw runtime_error("xcb_get_property returned nullptr and no error");
	}

	// no need to free the value?
	return std::string(reinterpret_cast<char*>(xcb_get_property_value(title_reply.get())));
}
