#include "foreign_window.hpp"
#include "private_xcb.h"

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl) {
	*impl = impl_; /* you know what you're doing */
}
