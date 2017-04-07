#include <xcb/xcb.h>

struct ForeignWindowImpl {
	xcb_window_t wid;
	xcb_connection_t* conn;
};

struct XCB_atoms { // WindowWatcher should fill it while being constructed
	static xcb_atom_t NET_ACTIVE_WINDOW;
	static xcb_atom_t NET_WM_NAME;
	static xcb_atom_t CARDINAL;
	static xcb_atom_t UTF8_STRING;
};
