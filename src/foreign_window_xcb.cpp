#include "foreign_window.hpp"
#include <xcb/xcb.h>

struct ForeignWindowImpl {
	xcb_window_t wid;
};
