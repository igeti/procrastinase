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
	timer& add_abs(std::chrono::steady_clock::time_point when, T cb) {
		/*
		 * After changing timers,
		 * recalculate next_wakeup,
		 * set changes_pending = true,
		 * wait on change_noted while changes_pending.
		 */
	}
	thr_timer()
		: lock(mutex), changes_pending(false), terminating(false),
		next_wakeup(std::chrono::steady_clock::now()), waiter(loop, this, next_wakeup) {}
	~thr_timer() {
		terminating = true;
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
		waiter.detach();
	}
private:
	std::timed_mutex mutex;
	std::unique_lock<std::timed_mutex> lock; // keep mutex locked by owning thread most of the time
	bool changes_pending;
	bool terminating;
	std::chrono::steady_clock::time_point next_wakeup;
	std::condition_variable change_noted;
	std::list<timer> timers;
	std::thread waiter;
	void loop(std::chrono::steady_clock::time_point wakeup) {
		for(/**/;;/**/) { // <-- behold the ASCII Cthulhu
		/*
		 * Most of the time, keep trying to unlock the mutex with timeout
		 * set to the closest timer expiration.
		 */
		// wakeup in the past means that there are no timers set => just keep waiting
			if (
				std::chrono::steady_clock::now() > wakeup
				? mutex.try_lock_until(wakeup)
				: mutex.try_lock()
			) {
				/*
				 * If succeeded, some change has occured and we need to record
				 * the new timeout, set changes_pending=false, fire change_noted,
				 * then resume locking. If terminating, return.
				 */
				 // don't forget
				 mutex.unlock();
			} else {
			/*
			 * If failed, timer has fired and we need to call the callback.
			 */
			}
		}
	}
};

