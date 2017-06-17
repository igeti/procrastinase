#define _POSIX_C_SOURCE 200809L // readlink
#include "foreign_window.hpp"
#include "private_xcb.hpp"
#include <stdexcept> // runtime_error
#include <memory> // unique_ptr
#include <sys/types.h> // pid_t
#include <stdlib.h> // realpath
#include <signal.h> // kill
#include <algorithm> // find

const size_t title_path_length = 4096; // VFS has troubles coping with >256-character paths, anyway

ForeignWindowImpl::ForeignWindowImpl(xcb_connection_t* conn_, xcb_window_t wid_) :conn(conn_), wid(wid_) {}

ForeignWindow::ForeignWindow(ForeignWindow && fw) :impl(std::move(fw.impl)) {}

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl(impl_)) {
	try {
		// according to libxcb-ewmh headers, CARDINAL == uint32_t
		impl->pid = impl->get_property<uint32_t>(XCB::NET_WM_PID, XCB_ATOM_CARDINAL);
		// now store the uint32_t in a pid_t
	} catch (std::runtime_error & err) {
		// some windows (e.g. Worker) just don't have _NET_WM_PID set
		// also, this could be a remote window, if the user had enough reasons to use them
		impl->pid = 0;
	}
}

std::string ForeignWindow::get_window_title() const {
	return impl->get_property(XCB::NET_WM_NAME, title_path_length);
}

static std::string executable_path(pid_t pid) { // XXX: this is Linux-only, see sysctl calls on *BSD and proc_pidpath on macOS
	std::string link {"/proc/"};
	link += std::to_string(pid);
	link += "/exe";
	std::unique_ptr<char,decltype(&std::free)> path{realpath(link.c_str(),nullptr),&std::free};
	if (!path) throw std::runtime_error("realpath returned error");
	return std::string(path.get());
}

std::string ForeignWindow::get_program_path() const {
	try {
		if (impl->pid)
			return executable_path(impl->pid);
	} catch (std::runtime_error &) {} // paranoid kernel doesn't let us peek at /proc? oh well
	// anyway, if we've got here, we can't know the PID of the window owner
	// get the full WM_CLASS value instead
	std::string wm_class = impl->get_property(XCB_ATOM_WM_CLASS,title_path_length,XCB_ATOM_STRING);
	// cut the "instance class" (before \0) and leave the "app class" (after first \0)
	wm_class.erase(wm_class.begin(), std::find(wm_class.begin(), wm_class.end(), '\0'));
	return wm_class;
}

void ForeignWindow::kill() {
	if (impl->pid && !::kill(impl->pid,SIGTERM))
			return; // we successfully killed the process by its PID
	// if we're still here, let's try to kill the window instead
	// X11 has no security between clients, so we should probably succeed (but the app owning the window may be very surprised)
	xcb_void_cookie_t kill_cookie = xcb_kill_client_checked(impl->conn, impl->wid);
	std::unique_ptr<xcb_generic_error_t,decltype(&free)> error {xcb_request_check(impl->conn, kill_cookie),&free};
	// man xcb-requests says I should free result of xcb_request_check
	if (error)
		throw std::runtime_error("xcb_kill_client returned error");
}

// work around unique_ptr asking for destructor while compiling public headers
ForeignWindow::~ForeignWindow() = default;
