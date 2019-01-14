#pragma once

#include <queue>
#include <mutex>

using std::queue;
using std::mutex;
using std::unique_lock;
using std::condition_variable;

template<typename T>
class thread_safe_queue {
public:
	thread_safe_queue() = default;

	thread_safe_queue(thread_safe_queue& queue) = delete;

	thread_safe_queue(thread_safe_queue&& moveable_queue) {
		unique_lock<mutex> queue_mutex_unique_lock(queue_mutex);
		item_queue = move(moveable_queue.item_queue);
	}

	void enqueue(T item) {
		unique_lock<mutex> queue_mutex_unique_lock(queue_mutex);
		item_queue.push(item);
		cond.notify_one();
	};

	T dequeue() {
		unique_lock<mutex> queue_mutex_unique_lock(queue_mutex);
		while (item_queue.empty()) {
			cond.wait(queue_mutex_unique_lock);
		}

		T item_to_return = item_queue.front();
		item_queue.pop();

		return item_to_return;
	};

private:
	queue<T> item_queue;
	mutex queue_mutex;
	condition_variable cond;
};