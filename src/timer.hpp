#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <list>

template <class T>
class thr_timer {
public:
	class timer {
		std::chrono::steady_clock::duration time;
		T cb;
	};
	timer& add_rel(std::chrono::steady_clock::duration diff, T cb) {
		return add_abs(std::chrono::steady_clock::now() + diff, cb);
	}
	timer& add_abs(std::chrono::steady_clock::time_point when, T cb) {}
	thr_timer() : waiter(loop, this) {
		/*
		 * Keep it locked for most of the time.
		 * After changing timers,
		 * unlock it,
		 * set changes_pending = true,
		 * wait on change_noted while changes_pending.
		 */
		lock.lock();
	}
private:
	std::timed_mutex lock;
	bool changes_pending;
	std::condition_variable change_noted;
	std::list<timer> timers;
	std::thread waiter;
	void loop() {
		/*
		 * Most of the time, keep trying to unlock the mutex with timeout
		 * set to the closest timer expiration.
		 * If succeeded, some change has occured and we need to recalculate
		 * the timeout, set changes_pending=false and fire change_noted.
		 * If failed, timer has fired and we need to call the callback.
		 */
	}
};

