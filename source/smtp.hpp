// smtp.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "socket.hpp"
#include "config.hpp"
#include "util_synch.hpp"
#include "util_win32.hpp"


class smtp;

namespace sync
{

inline lock<win32::critical_section> acquire(smtp&);

// unavailable, because critical_section does not support timed wait
// inline lock<win32::critical_section> try_acquire(smtp& s, unsigned long t);

inline lock<win32::critical_section> try_acquire(smtp& s);

} // namespace sync


class smtp : public tcp::socket<smtp>
{
public:
	struct error : public tcp::error
	{
		explicit error(const char* msg) : tcp::error(msg) {}
	};

private:
	// non-copyable and non-assignable
	smtp(const smtp&);
	smtp& operator=(const smtp&);

	friend class tcp::socket<smtp>;

	static const unsigned int buffer_size_ = 2000;
	static const unsigned int close_timeout_ = 500;
	static const bool send_bye_command_ = true;
	static const bool close_gracefully_ = true;
	
	static const unsigned int minimum_partial_response_ = 4;

	timer timer_;
	unsigned long max_req_time_ms_;
	std::vector<std::string> response_;
	std::string partial_response_;
	win32::critical_section socket_lock_;
	win32::critical_section disconnecting_;
	unsigned long idle_timeout_s_;
	bool connected_;
	unsigned long max_connection_s_;
	timer connection_timer_;
	win32::handle thread_;
	win32::handle event_;

	// synchronization interface
	friend inline sync::lock<win32::critical_section> sync::acquire(smtp&);

	friend inline sync::lock<win32::critical_section> sync::try_acquire(smtp&);

	bool on_is_alive(const tcp::ip4_host& s);

	bool on_recv(char* data, unsigned int len);

	unsigned int recv();

	friend DWORD WINAPI idle_thread(void* pv);

public:
	const config configuration;

	explicit smtp(const config& c);

	~smtp()
	{
		disc();
		WaitForSingleObject(*thread_, INFINITE);
	}

	void reset_timer(unsigned long max)
	{
		max_req_time_ms_ = max;
		timer_.reset();
	}

	void reset_timer()
	{
		timer_.reset();
	}

	void send(const std::string& data)
	{
		try
		{
			const sync::scoped_lock& g = sync::acquire(socket_lock_);
			SetEvent(*event_);
			tcp::socket<smtp>::send(data, timer_, max_req_time_ms_);
		}
		catch (std::exception&)
		{
			disc();
			throw;
		}
	}

	unsigned int send_recv(const std::string& data)
	{
		try
		{
			const sync::scoped_lock& g = sync::acquire(socket_lock_);
			SetEvent(*event_);
			tcp::socket<smtp>::send(data, timer_, max_req_time_ms_);
			return recv();
		}
		catch (std::exception&)
		{
			disc();
			throw;
		}
	}

	void disc()
	{
		const sync::scoped_lock& g = sync::acquire(socket_lock_);
		const sync::scoped_lock& d = sync::acquire(disconnecting_);
		connected_ = false;

		SetEvent(*event_);
		tcp::socket<smtp>::disc("QUIT\r\n");
	}

	bool is_alive(const config& c)
	{
		return tcp::socket<smtp>::is_alive(tcp::ip4_host(c.server_address, c.server_port));
	}
};

namespace sync
{

inline lock<win32::critical_section> acquire(smtp& s)
{
	return lock<win32::critical_section>(s.socket_lock_);
}

inline lock<win32::critical_section> try_acquire(smtp& s)
{
	return lock<win32::critical_section>(s.socket_lock_, lock<win32::critical_section>::dont_wait());
}

} // namespace sync

