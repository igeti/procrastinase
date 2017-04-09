#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <list>
#include <algorithm>
#include <cassert>

template <class T>
class thr_timer {
public:
	class timer {
		std::chrono::steady_clock::duration time;
		T & cb;
		timer(std::chrono::steady_clock::duration time_, T & cb_) : time(time_), cb(cb_) {}
		void cancel() { throw "TODO: unimplemented"; }
	};
	timer & add_rel(std::chrono::steady_clock::duration diff, T & cb) {
		return add_abs(std::chrono::steady_clock::now() + diff, cb);
	}
	timer& add_abs(std::chrono::steady_clock::time_point when, T & cb) {
		{
			std::unique_lock<std::mutex> lock(list_mutex); // list access is additionally guarded
			timers.push_back({when, cb});
			next_wakeup = timers.front().when;
			for (timer& t: timers)
				if (t.when < next_wakeup) next_wakeup = t.when;
		}
		/*
		 * After changing timers,
		 * recalculate next_wakeup,
		 * set changes_pending = true,
		 * wait on change_noted while changes_pending.
		 */
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
	}
	thr_timer()
		: lock(tmutex), changes_pending(false), terminating(false),
		next_wakeup(std::chrono::steady_clock::now()), waiter(loop, this, next_wakeup) {}
	~thr_timer() {
		terminating = true;
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
		waiter.detach();
	}
private:
	std::timed_mutex tmutex;
	std::mutex list_mutex;
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
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		// wakeup in the past means that there are no timers set => just keep waiting
			if (
				now > wakeup
				? tmutex.try_lock_until(wakeup)
				: tmutex.try_lock()
			) {
				/*
				 * If succeeded, some change has occured and we need to record
				 * the new timeout, set changes_pending=false, fire change_noted,
				 * then resume locking. If terminating, return.
				 */
				 wakeup = next_wakeup;
				 changes_pending = false;
				 bool local_terminating = terminating; // can't use terminating after unlock
				 tmutex.unlock();
				 change_noted.notify_one();
				 if (local_terminating) return;
			}
			/*
			 * If failed, timer has fired and we need to call the callback(s).
			 * Even if not, check for expired timers anyway. Otherwise, it
			 * would be possible to set a timer in the past and have it never fire
			 * and clog the queue because wakeup < now.
			 */
			{
				std::unique_lock<std::mutex> lock(list_mutex);
				if (timers.empty()) continue;
				while (
					auto t = std::find_if(
						timers.begin(), timers.end(),
						[now](timer&t){ t.time <= now; }
					) != timers.end()
				) {
					t->cb();
					t.erase(t);
				}
			}
		}
	}
};
