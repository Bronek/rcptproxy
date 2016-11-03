// smtp.cpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"
#include "smtp.hpp"

smtp::smtp(const config& c) :
	tcp::socket<smtp>(tcp::ip4_host(c.server_address, c.server_port)),
	max_req_time_ms_(c.request_max_delay),
	idle_timeout_s_(c.conn_idle_timeout),
	connected_(true),
	max_connection_s_(c.conn_max_time),
	configuration(c)
{
	if (recv() >= 300)
		throw error("Protocol error : remote server is not ready");
	if (send_recv(std::string("HELO ") + c.protocol_helo + "\r\n") >= 300)
		throw error("Protocol error : remote server is not ready");

	event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (*event_ == NULL)
		throw error("Unable to create wait object");
	
	DWORD id;
	thread_ = CreateThread(NULL, 0, &idle_thread, this, 0, &id);
	if (*thread_ == NULL)
		throw error("Unable to create idle thread");
}

bool smtp::on_is_alive(const tcp::ip4_host& s)
{
	try
	{
		const sync::scoped_lock& g = sync::acquire(socket_lock_);

		reset_timer();
		if (send_recv("RSET\r\n") > 300)
		{
			disc();
			return false;
		}
	}
	catch (std::exception&)
	{
		return false;
	}
	
	return true;
}


bool smtp::on_recv(char* data, unsigned int len)
{
	for (unsigned int i = 0; i < len; ++i)
	{
		if (data[i] >= ' ') 
			partial_response_ += data[i];
		else if (data[i] == '\n')
		{
			response_.push_back(partial_response_);
			partial_response_.clear();
		}
	}

	if (!partial_response_.empty() || response_.size() == 0)
		return true;

	const std::string& last = response_[response_.size() - 1];
	if (last.size() < minimum_partial_response_)
		throw error("Protocol error : response from SMTP server is too short");
	else if (last[3] == '-')
		return true;
	else  if (last[3] == ' ')
		return false;
	else
		throw error("Protocol error : response from SMTP server is invalid");
}


unsigned int smtp::recv()
{
	try
	{
		const sync::scoped_lock& g = sync::acquire(socket_lock_);
		SetEvent(*event_);

		std::vector<std::string>().swap(response_);
		partial_response_.clear();

		tcp::socket<smtp>::recv(timer_, max_req_time_ms_);

		if (response_.size() == 0)
			throw error("Protocol error : no response from SMTP server");

		const std::string& last = response_[response_.size() - 1];
		if (last.size() < minimum_partial_response_)
			throw error("Protocol error : response from SMTP server is too short");
		if (last[0] > '5' || last[0] < '2' ||
			last[1] > '9' || last[1] < '0' ||
			last[2] > '9' || last[2] < '0')
			throw error("Protocol error : response from SMTP server is invalid");

		return (last[0] - '0') * 100
			+ (last[1] - '0') * 10
			+ (last[2] - '0');
	}
	catch (std::exception&)
	{
		disc();
		throw;
	}
}


DWORD WINAPI idle_thread(void* pv)
{
	smtp* me = reinterpret_cast<smtp*> (pv);

	unsigned long t = std::min(me->idle_timeout_s_, static_cast<unsigned long> (me->connection_timer_.s(me->max_connection_s_)));
	while (WaitForSingleObject(*me->event_, 1000L * t) != WAIT_TIMEOUT)
	{
		const sync::scoped_lock& g = sync::acquire(me->disconnecting_);

		if (!me->connected_)
			return 0;

		g.unlock();

		t = std::min(me->idle_timeout_s_, static_cast<unsigned long> (me->connection_timer_.s(me->max_connection_s_)));
	}

	me->disc();
	return 0;
}
