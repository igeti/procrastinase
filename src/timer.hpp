#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <queue>
#include <algorithm>
#include <cassert>

#include <iostream>
template<class T> void hexdump(const T & arg) {
	unsigned char* wtf = (unsigned char*)&arg;
	std::cerr << (void*)wtf << "\t";
	for (size_t i = 0; i < sizeof(T); i++)
		std::cerr << std::hex << (int)wtf[i];
	std::cerr << std::endl;
}

template <class T>
class thr_timer {
public:
	class timer {
	private:
		friend class thr_timer;
		std::chrono::steady_clock::time_point time;
		T * cb;
		timer(std::chrono::steady_clock::time_point time_=std::chrono::steady_clock::now(), T * cb_=nullptr)
			: time(time_), cb(cb_) {}
	public:
		bool operator >(const timer& other) const {
			return (time > other.time);
		}
		bool operator ==(const timer& other) const {
			hexdump(*this);
			hexdump(other);
			hexdump((time == other.time) && (cb == other.cb)); // why the FUCK does it not std::remove a timer after I return true?
			return (time == other.time) && (cb == other.cb);
		}
		operator bool() const { // timer is "dead" if callback is empty
			return cb;
		}
	};
	timer add_rel(std::chrono::steady_clock::duration diff, T & cb) {
		return add_abs(std::chrono::steady_clock::now() + diff, cb);
	}
	timer add_abs(std::chrono::steady_clock::time_point when, T & cb) {
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
		while (changes_pending) change_noted.wait(lock);
		// return a copy because queue+vector invalidates pointers/references on a whim
		return {when, &cb};
	}
	void cancel(timer & t) {
		if (!t) return;
		std::unique_lock<std::mutex> lock(list_mutex);
		timers.remove(t);
		t.cb = nullptr;
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
			std::remove(c.begin(), c.end(), val);
		}
	};
	std::timed_mutex tmutex;
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
				wait_indefinite = timers.empty();
				if (!wait_indefinite)
					wait_until = timers.top().time;
			}
			bool locked = false;
			if (wait_indefinite) {
				tmutex.lock();
				locked = true;
			} else {
				locked = tmutex.try_lock_until(wait_until);
			}
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
