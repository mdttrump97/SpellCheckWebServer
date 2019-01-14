#pragma once

#include "request.hpp"
#include "reply.hpp"
#include "server.hpp"

class task {
public:
	task(const http::server::request& req, http::server::reply& rep, http::server::done_callback done) : req(req), rep(rep), done(done) {};
	const http::server::request& req;
	http::server::reply& rep;
	http::server::done_callback done;
};