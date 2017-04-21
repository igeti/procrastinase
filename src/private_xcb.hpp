#include <xcb/xcb.h>
#include <memory>
#include <stdexcept>

struct ForeignWindowImpl {
	xcb_connection_t* conn;
	xcb_window_t wid;
	pid_t pid;
	ForeignWindowImpl(xcb_connection_t*, xcb_window_t);
};

struct XCB { // WindowWatcher should fill this while being constructed
	static xcb_atom_t NET_ACTIVE_WINDOW;
	static xcb_atom_t NET_WM_NAME;
	static xcb_atom_t UTF8_STRING;
	static xcb_atom_t NET_WM_PID;

	// FIXME: this should probably be a ForeignWindow method
	template<typename ret>
	static ret get_property(xcb_connection_t* conn, xcb_window_t wid, xcb_atom_t atom, xcb_atom_t type, size_t len = sizeof(ret)) {
		return *reinterpret_cast<ret*>(
			xcb_get_property_value(
				get_property_reply(conn, wid, atom, type, len).get()
			)
		);
	}

	static std::string get_property(xcb_connection_t* conn, xcb_window_t wid, xcb_atom_t atom, xcb_atom_t type, size_t len) {
		return std::string(
			reinterpret_cast<char*>(
					xcb_get_property_value(
						get_property_reply(conn, wid, atom, type, len).get()
					)
			)
		);
	}
private:
	static std::unique_ptr<xcb_get_property_reply_t,decltype(&std::free)>
	get_property_reply(xcb_connection_t* conn, xcb_window_t wid, xcb_atom_t atom, xcb_atom_t type, size_t len) {
		using std::unique_ptr;
		using std::runtime_error;
		xcb_generic_error_t *err = nullptr; // can't use unique_ptr here because get_property_reply overwrites pointer value

		/*
		Specifies how many 32-bit multiples of data should be retrieved
		(e.g. if you set long_length to 4, you will receive 16 bytes of data).
		*/
		uint32_t ret_size = len/sizeof(uint32_t)/*integer division, like floor()*/ + !!(len%sizeof(uint32_t))/*+1 if there was a remainder*/;

		xcb_get_property_cookie_t cookie = xcb_get_property(
			conn, 0, wid, atom, type, 0, ret_size
		);
		unique_ptr<xcb_get_property_reply_t,decltype(&std::free)> reply {xcb_get_property_reply(conn, cookie, &err),&free};
		if (!reply) {
			if (err) {
				free(err);
				throw runtime_error("xcb_get_property returned error");
			} else throw runtime_error("xcb_get_property returned nullptr and no error");
		}
		return reply;
	}
};
