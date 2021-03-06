#include <xcb/xcb.h>
#include <memory>
#include <stdexcept>

struct XCB {
	static xcb_atom_t NET_ACTIVE_WINDOW;
	static xcb_atom_t NET_WM_NAME;
	static xcb_atom_t UTF8_STRING;
	static xcb_atom_t NET_WM_PID;
};

struct ForeignWindowImpl : public XCB {
	xcb_connection_t* conn;
	xcb_window_t wid;
	pid_t pid;
	ForeignWindowImpl(xcb_connection_t*, xcb_window_t);

	template<typename T>
	T get_property(xcb_atom_t atom, xcb_atom_t type, size_t len = sizeof(T)) {
		return *reinterpret_cast<T*>(
			xcb_get_property_value(
				get_property_reply(atom, type, len).get()
			)
		);
	}

	std::string get_property(xcb_atom_t atom, size_t len, xcb_atom_t type = UTF8_STRING) {
		auto reply = get_property_reply(atom, type, len);
		return std::string(
			reinterpret_cast<char*>(
				xcb_get_property_value(
					reply.get()
				)
			),
			(size_t)xcb_get_property_value_length(reply.get()) // WHY should it have been int?
		);
	}
private:
	std::unique_ptr<xcb_get_property_reply_t,decltype(&std::free)>
	get_property_reply(xcb_atom_t atom, xcb_atom_t type, size_t len) {
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
			free(err);
			throw runtime_error("xcb_get_property returned error");
		}

		if (len && !xcb_get_property_value_length(reply.get()))
			throw runtime_error("xcb_get_property returned empty reply when requested size != 0");

		return reply;
	}
};
