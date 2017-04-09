#include <xcb/xcb.h>

struct ForeignWindowImpl {
	xcb_connection_t* conn;
	xcb_window_t wid;
	pid_t pid;
	ForeignWindowImpl(xcb_connection_t*, xcb_window_t);
};

struct XCB_atoms { // WindowWatcher should fill this while being constructed
	static xcb_atom_t NET_ACTIVE_WINDOW;
	static xcb_atom_t NET_WM_NAME;
	static xcb_atom_t UTF8_STRING;
	static xcb_atom_t NET_WM_PID;
};
