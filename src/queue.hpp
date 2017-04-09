#include <mutex>
#include <condition_variable>
#include <queue>
#include <utility>
#include <stdexcept>

struct thr_finished : public std::runtime_error {
	thr_finished() : std::runtime_error("thr_queue finished") {}
};

template <typename elt>
class thr_queue {
private:
	std::queue<elt> queue;
	std::mutex mutex;
	std::condition_variable cv;
	bool finished;
public:
	thr_queue() : finished(false) {}

	void push(const elt & element) { // creates a copy of existing object
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (finished) throw thr_finished();
			queue.push(element);
		}
		cv.notify_one();
	}

	void push(elt && element) { // moves temp objects
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (finished) throw thr_finished();
			queue.push(std::move(element)); // named && are treated as &
		}
		cv.notify_one();
	}

	void finish() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			finished = true;
		}
		cv.notify_all();
	}

	elt pop() {
		std::unique_lock<std::mutex> lock(mutex);
		while (queue.empty() && !finished) cv.wait(lock);
		if (finished) throw thr_finished();
		elt ret(std::move(queue.front()));
		queue.pop(); // destroys front
		lock.unlock();
		return ret;
	}

	// should be referenced, not copied
	// use std::ref when creating std::thread
	thr_queue(const thr_queue&) = delete;
	thr_queue& operator=(const thr_queue&) = delete;
};
