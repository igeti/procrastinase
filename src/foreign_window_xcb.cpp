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
	impl->pid = XCB::get_property<uint32_t>(impl->conn, impl->wid, XCB::NET_WM_PID, XCB_ATOM_CARDINAL);
	// now store the uint32_t in a pid_t
}

std::string ForeignWindow::get_window_title() {
	return XCB::get_property(impl->conn, impl->wid, XCB::NET_WM_NAME, XCB::UTF8_STRING, title_path_length);
}

static std::string executable_path(pid_t pid) { // FIXME: Linux-only, see sysctl calls on *BSD and proc_pidpath on macOS
	std::string link {"/proc/"};
	link += pid;
	link += "/exe";
	std::unique_ptr<char,decltype(&std::free)> path{realpath(link.c_str(),nullptr),&std::free};
	if (!path) throw std::runtime_error("realpath returned error");
	return std::string(path.get());
}

std::string ForeignWindow::get_program_path() {
	return executable_path(impl->pid);
}

void ForeignWindow::kill() {
	if (::kill(impl->pid,SIGTERM))
		throw std::runtime_error("kill returned error");
}

// work around unique_ptr asking for destructor while compiling public headers
ForeignWindow::~ForeignWindow() = default;
