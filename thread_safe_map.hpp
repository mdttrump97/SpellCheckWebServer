#pragma once

#include<map>
#include<mutex>
#include<shared_mutex>
#include<iostream>
#include<vector>

using std::map;
using std::shared_mutex;
using std::shared_lock;
using std::unique_lock;
using std::cout;
using std::endl;
using std::vector;

template<typename T1, typename T2>
class thread_safe_map {
public:
	thread_safe_map() = default;

	thread_safe_map(thread_safe_map& map) = delete;

	thread_safe_map(thread_safe_map&& moveable_map) {
		unique_lock<shared_mutex> ul(map_mutex);
		safe_map = move(moveable_map.safe_map);
	}

	T2 operator[](T1 key) {
		shared_lock<shared_mutex> sl(map_mutex);
		return safe_map[key];
	}

	bool contains(T1 key) {
		shared_lock<shared_mutex> sl(map_mutex);
		bool in_map = false;
		if (safe_map.find(key) != safe_map.end()) {
			in_map = true;
		}
		return in_map;
	}

	void write(T1 key, T2 value) {
		unique_lock<shared_mutex> ul(map_mutex);
		safe_map[key] = value;
	}

	vector<T1> get_keys() {
		shared_lock<shared_mutex> sl(map_mutex);
		vector<T1> keys;
		for (auto i = safe_map.begin(); i != safe_map.end(); ++i) {
			keys.push_back((*i).first);
		}
		return keys;
	}

	void erase(T1 key) {
		unique_lock<shared_mutex> ul(map_mutex);
		safe_map.erase(key);
	}

private:
	map<T1, T2> safe_map;
	shared_mutex map_mutex;
};