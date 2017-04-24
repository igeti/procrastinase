#define _POSIX_C_SOURCE 200809L // readlink
#include "foreign_window.hpp"
#include "private_xcb.hpp"
#include <stdexcept>
#include <memory>
#include <sys/types.h> // pid_t
#include <cmath> // ceil
#include <stdlib.h> // realpath
#include <signal.h> // kill

const size_t title_path_length = 4096; // VFS has troubles coping with >256-character paths, anyway

ForeignWindowImpl::ForeignWindowImpl(xcb_connection_t* conn_, xcb_window_t wid_) :conn(conn_), wid(wid_) {}

ForeignWindow::ForeignWindow(ForeignWindow && fw) :impl(std::move(fw.impl)) {}

ForeignWindow::ForeignWindow(ForeignWindowImpl impl_) :impl(new ForeignWindowImpl(impl_)) {
	// according to libxcb-ewmh headers, CARDINAL == uint32_t
	impl->pid = impl->get_property<uint32_t>(XCB::NET_WM_PID, XCB_ATOM_CARDINAL); // FIXME: for windows without _NET_WM_PID, garbage is returned
	// now store the uint32_t in a pid_t
}

std::string ForeignWindow::get_window_title() const {
	return impl->get_property(XCB::NET_WM_NAME, title_path_length);
}

static std::string executable_path(pid_t pid) { // FIXME: Linux-only, see sysctl calls on *BSD and proc_pidpath on macOS
	std::string link {"/proc/"};
	link += std::to_string(pid);
	link += "/exe";
	std::unique_ptr<char,decltype(&std::free)> path{realpath(link.c_str(),nullptr),&std::free};
	if (!path) throw std::runtime_error("realpath returned error");
	return std::string(path.get());
}

std::string ForeignWindow::get_program_path() const {
	if (impl->pid)
		return executable_path(impl->pid);
	else
		return impl->get_property(XCB_ATOM_WM_CLASS,title_path_length);
}

void ForeignWindow::kill() {
	if (impl->pid) { // kill process
		if (::kill(impl->pid,SIGTERM))
			throw std::runtime_error("kill returned error");
		return;
	} else { // kill window
		xcb_void_cookie_t kill_cookie = xcb_kill_client_checked(impl->conn, impl->wid);
		xcb_generic_error_t * error = xcb_request_check(impl->conn, kill_cookie);
		if (error) {
			free(error); // FIXME: XCB doesn't clearly specify if I should
			throw std::runtime_error("xcb_kill_client returned error");
		}
	}
}

// work around unique_ptr asking for destructor while compiling public headers
ForeignWindow::~ForeignWindow() = default;
