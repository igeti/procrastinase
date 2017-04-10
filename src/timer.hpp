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
		friend class thr_timer;
		std::chrono::steady_clock::time_point time;
		T & cb;
		timer(std::chrono::steady_clock::time_point time_, T & cb_)
			: time(time_), cb(cb_) {}
	public:
		bool operator==(const timer& other) {
			return (time == other.time) && (&cb == &other.cb);
		}
	};
	timer * add_rel(std::chrono::steady_clock::duration diff, T & cb) {
		return add_abs(std::chrono::steady_clock::now() + diff, cb);
	}
	timer * add_abs(std::chrono::steady_clock::time_point when, T & cb) {
		timer * ret = nullptr;
		{
			std::unique_lock<std::mutex> lock(list_mutex);
			timers.push_back({when, cb});
			recalculate_wakeup();
			ret = &timers.back();
		}
		/*
		 * After changing timers,
		 * recalculate next_wakeup,
		 * set changes_pending = true,
		 * wait on change_noted while changes_pending.
		 */
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
		return ret;
	}
	void cancel(timer ** t) {
		assert(t);
		std::unique_lock<std::mutex> lock(list_mutex);
		timers.remove(**t);
		*t = nullptr;
		recalculate_wakeup();
	}
	thr_timer()
		: lock(tmutex), changes_pending(false), terminating(false),
		next_wakeup(std::chrono::steady_clock::now()), waiter(&thr_timer::loop, this, next_wakeup) {}
	~thr_timer() {
		terminating = true;
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
		waiter.detach();
	}
private:
	std::timed_mutex tmutex;
	std::mutex list_mutex; // list access is additionally guarded
	std::unique_lock<std::timed_mutex> lock; // keep mutex locked by owning thread most of the time
	bool changes_pending;
	bool terminating;
	std::chrono::steady_clock::time_point next_wakeup;
	std::condition_variable_any change_noted;
	std::list<timer> timers;
	std::thread waiter;
	void recalculate_wakeup() {
		if (timers.empty()) return;
		next_wakeup = timers.front().time;
		for (timer& t: timers)
			if (t.time < next_wakeup) next_wakeup = t.time;
	}
	void loop(std::chrono::steady_clock::time_point wakeup) {
		for(/**/;;/**/) { // <-- behold the ASCII Cthulhu
			/*
			 * Most of the time, keep trying to unlock the mutex with timeout
			 * set to the closest timer expiration.
			 */
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			bool locked = false;
			// wakeup in the past means that there are no timers set => just keep waiting until something changes
			if (now < wakeup) {
				locked = tmutex.try_lock_until(wakeup);
			} else {
				tmutex.lock();
				locked = true;
			}
			now = std::chrono::steady_clock::now(); // it might have taken a lot of time to get there
			if (locked) {
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
				now = std::chrono::steady_clock::now(); // and maybe here, too
				if (timers.empty()) continue;
				for (
					auto t = std::find_if(
						timers.begin(), timers.end(),
						[now](timer&t){ return t.time <= now; }
					)
					;t != timers.end();
					t = std::find_if(
						timers.begin(), timers.end(),
						[now](timer&t){ return t.time <= now; }
					)
				) {
					t->cb();
					timers.erase(t);
				}
			}
		}
	}
};
