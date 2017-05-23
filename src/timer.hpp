#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <queue>
#include <algorithm>
#include <cassert>

/*
Usage:

1) Create a thr_timer tmr;
2) Use tmr.add_rel or tmr.add_abs to add timers and remember their return values.
Don't feed temporary cb objects because timer only remembers pointers to them.
3) Use tmr.cancel(timer) to cancel existing timers, if you need to. After cancellation, you can
assign a different timer to the same variable. Use operator bool overload to
check if a given timer is active (not cancelled yet). Cancelling a dead timer is no-op.
4) Once thr_timer moves out of scope, it stops the watcher threads and cancels
the remaining timers.
*/

class thr_timer {
public:
	class timer {
	private:
		friend class thr_timer;
		std::chrono::steady_clock::time_point time;
		std::function<void()> * cb;
		timer(std::chrono::steady_clock::time_point time_=std::chrono::steady_clock::now(), std::function<void()> * cb_=nullptr)
			: time(time_), cb(cb_) {}
	public:
		bool operator >(const timer& other) const {
			return (time > other.time);
		}
		bool operator ==(const timer& other) const {
			return (time == other.time) && (cb == other.cb);
		}
		operator bool() const { // timer is "dead" if callback is empty
			return cb;
		}
	};
	timer add_rel(std::chrono::steady_clock::duration diff, std::function<void()> & cb) {
		return add_abs(std::chrono::steady_clock::now() + diff, cb);
	}
	timer add_abs(std::chrono::steady_clock::time_point when, std::function<void()> & cb) {
		{
			std::unique_lock<std::mutex> lock(list_mutex);
			timers.push({when, &cb});
		}
		/*
		 * After changing timers,
		 * set changes_pending = true,
		 * wait on change_noted while changes_pending.
		 */
		changes_pending = true;
		// wake up the waiting thread so it would set the appropriate timeout
		while (changes_pending) change_noted.wait(lock);
		// return a copy because queue+vector invalidates pointers/references on a whim
		return {when, &cb};
	}
	void cancel(timer & t) {
		if (!t) return;
		std::unique_lock<std::mutex> lock(list_mutex);
		timers.remove(t);
		t.cb = nullptr;
		// no need to wake up the waiting thread specifically: it'll just note that
		// the stars aren't right yet and go back to sleep
	}
	thr_timer()
		: lock(tmutex), changes_pending(false), terminating(false),
		waiter(&thr_timer::loop, this) {}
	~thr_timer() {
		terminating = true;
		changes_pending = true;
		while (changes_pending) change_noted.wait(lock);
		waiter.detach();
	}
private:
	template <class W, class U, class V>
	class editable_priority_queue : public std::priority_queue<W,U,V> {
	private:
		using std::priority_queue<W,U,V>::c;
	public:
		void remove(const W & val) {
			c.erase(std::remove(c.begin(), c.end(), val), c.end());
		}
	};
	/*
	How It Works:
	This class abuses the ability of timed_mutex to wait for an opening to lock it.
	By setting a timeout for the next timer and trying to lock this mutex, it's
	able to be interrupted by the main thread releasing the lock.
	*/
	std::timed_mutex tmutex;
	/*
	Problem: most of the interesting stuff happens with tmutex owned by main thread
	(on lock timeout)
	Solution: guard the timer queue with another mutex
	*/
	std::mutex list_mutex; // list access is additionally guarded
	std::unique_lock<std::timed_mutex> lock; // keep mutex locked by owning thread most of the time
	bool changes_pending;
	bool terminating;
	std::condition_variable_any change_noted;
	editable_priority_queue<timer,std::vector<timer>,std::greater<timer>> timers;
	std::thread waiter;
	void loop() {
		for(/**/;;/**/) { // <-- behold the ASCII Cthulhu
			/*
			 * Most of the time, keep trying to unlock the mutex with timeout
			 * set to the closest timer expiration.
			 */
			std::chrono::steady_clock::time_point wait_until = std::chrono::steady_clock::now();
			bool wait_indefinite = true;
			{
				std::unique_lock<std::mutex> lock(list_mutex);
				wait_indefinite = timers.empty(); // no reason to wake up is there're no timers to fire
				if (!wait_indefinite)
					wait_until = timers.top().time;
			}
			bool locked = false;
			// now wait for next timer to fire / any change in the queue
			if (wait_indefinite) {
				tmutex.lock();
				locked = true;
			} else {
				locked = tmutex.try_lock_until(wait_until);
			}
			/*
			Note: we have to use this synchro stuff for 2 reasons:
			- mutexes issue memory barriers meaning that protected variables
			are guaranteed to have consistent values in caches of different CPUs
			- main thread needs to know when to lock the timed mutex again
			- this also means that I can't put the `bool terminating` to the while clause:
			it can be set from another thread, so no access before locking mutex
			*/
			if (locked) {
				/*
				 * Lock succeeded: maybe we're terminating?
				 */
				 changes_pending = false;
				 bool local_terminating = terminating; // can't use terminating after unlock
				 tmutex.unlock();
				 change_noted.notify_one();
				 if (local_terminating) return;
			}
			/*
			 * Whatever happens, check the queue for expired timers.
			 */
			{
				// now that we access the timer queue, lock the appropriate mutex
				// (not the timed one we've been abusing for side effects)
				std::unique_lock<std::mutex> lock(list_mutex);
				while (
					!timers.empty()
					&& timers.top().time <= std::chrono::steady_clock::now()
				) {
					assert(timers.top().cb);
					std::thread(*timers.top().cb).detach(); // whatever it was, start it in the background
					timers.pop();
				}
			}
		}
	}
};
