//
// main.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016 Gabriel Foust (gfoust at harding dot edu)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include "server.hpp"
#include "request.hpp"
#include "reply.hpp"
#include "spell.hpp"	
#include "task.h"
#include "thread_safe_queue.hpp"
#include <thread>
#include "thread_safe_map.hpp"
#include <vector>
#include <chrono>

using std::thread;
using std::string;
using std::vector;
using std::stringstream;
using std::chrono::high_resolution_clock;
using std::chrono::time_point;

thread_safe_queue<task> task_queue;
thread_safe_map <string, string> cache;
thread_safe_map <string, std::chrono::steady_clock::time_point> request_timestamps;

// Handle request by doing spell check on query string
// Render results as JSON
void spellcheck_request(const http::server::request& req, http::server::reply& rep) {
	// Set up reply
	rep.status = http::server::reply::status_type::ok;
	rep.headers["Content-Type"] = "application/json";

	if (cache.contains(req.query)) {
		rep.content << cache[req.query];
	}
	else {
		// Loop over spellcheck results
		bool first = true;
		rep.content << "[";
		for (auto& candidate : spell::spellcheck(req.query)) {
			if (first) {
				first = false;
			}
			else {
				rep.content << ", ";
			}
			rep.content << "\n  { \"word\" : \"" << candidate.word << "\",  "
				<< "\"distance\" : " << candidate.distance << " }";
		}
		rep.content << "\n]";

		cache.write(req.query, rep.content.str());
	}
}

void dump_cache(http::server::reply& rep) {
	rep.status = http::server::reply::status_type::ok;
	rep.headers["Content-Type"] = "application/json";
	for (auto key : request_timestamps.get_keys()) {
		rep.content << "\"" <<  key << "\"" << " last searched for " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - request_timestamps[key]).count() << " seconds ago" << endl;
	}
}

// Called by server whenever a request is received
// Must fill in reply, then call done()
void handle_request(const http::server::request& req, http::server::reply& rep, http::server::done_callback done) {
	task new_task(req, rep, done);
	task_queue.enqueue(new_task);
}

int test(int myInt) {
	return myInt + 1;
}

void process_request() {
	while (true) {
		task new_task = task_queue.dequeue();

		std::promise<int> myPromise;
		std::future<int> myFuture = myPromise.get_future(); 
		myPromise.set_value = test(2);
		cout << myFuture.get();

		std::cout << new_task.req.method << ' ' << new_task.req.uri << std::endl;
		if (new_task.req.path == "/spell") {
			high_resolution_clock clock;
			request_timestamps.write(new_task.req.query, clock.now());
			spellcheck_request(new_task.req, new_task.rep);
		}
		else if (new_task.req.path == "/cachedump") {
			dump_cache(new_task.rep);
		}
		else {
			new_task.rep = http::server::reply::stock_reply(http::server::reply::not_found);
		}
		new_task.done();
	}
}

void clear_cache() {
	while (true) {
		for (auto key : request_timestamps.get_keys()) {
			if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - request_timestamps[key]).count() > 60) {
				request_timestamps.erase(key);
				cache.erase(key);
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(60));
	}
}

// Initialize and run server
int main()
{
	try
	{
		for (size_t i = 0; i < thread::hardware_concurrency() - 2; i++) {
			std::cout << "Creating thread " << i << std::endl;
			thread t(process_request);
			t.detach();
		}

		thread cache_cleaner(clear_cache);
		cache_cleaner.detach();

		std::cout << "Listening on port 8000..." << std::endl;
		http::server::server s("localhost", "8000", handle_request);
		s.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}
	return 0;
}