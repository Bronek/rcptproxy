// config.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "metabase.hpp"

struct config
{
private:
	std::string						protocol_helo_;
	std::string						protocol_from_;
	std::string						rcpt_response_;

	std::vector<unsigned long>		exclusions_;
	mutable win32::critical_section	exc_lock_;

	void read_exclusions(metabase& md);

public:
	struct error : public std::runtime_error
	{
		explicit error(const char* msg) : std::runtime_error(msg) {}
	};

	struct complete_t{}; // Tag, just like std::nothrow_t

	static const complete_t complete;

	// read-only primitive types - safe for mutithreading without locks
	const bool						refresh;
	unsigned long					server_address;
	const unsigned short			server_port;
	const char* const				protocol_helo;
	const unsigned int				conn_idle_timeout;
	const unsigned int				conn_max_time;
	const bool						force_disconnect;
	const char* const				protocol_from;
	const unsigned int				request_max_delay;
	const bool						rcpt_append;
	const char* const				rcpt_response;
	unsigned int const				rcpt_status;

	// read limited configuration - IP, port and refresh req
	explicit config(metabase& mb);

	// read complete configuration
	config(metabase& mb, const complete_t&);

	config(const config& other) :
		protocol_helo_(other.protocol_helo_),
		protocol_from_(other.protocol_from_),
		rcpt_response_(other.rcpt_response_),
		exclusions_(other.exclusions_),
		refresh(other.refresh),
		server_address(other.server_address),
		server_port(other.server_port),
		protocol_helo(protocol_helo_.c_str()),
		conn_idle_timeout(other.conn_idle_timeout),
		conn_max_time(other.conn_max_time),
		force_disconnect(other.force_disconnect),
		protocol_from(protocol_from_.c_str()),
		request_max_delay(other.request_max_delay),
		rcpt_append(other.rcpt_append),
		rcpt_response(rcpt_response_.c_str()),
		rcpt_status(other.rcpt_status)
	{}

	bool is_excluded(unsigned long client_ip) const
	{
		const sync::scoped_lock& g = sync::acquire(exc_lock_);
		return std::binary_search(exclusions_.begin(), exclusions_.end(), client_ip);
	}
};

