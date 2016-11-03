// request.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "config.hpp"
#include "smtp.hpp"
#include "timer.hpp"

class request
{
	// non-copyable & non-assignable
	request& operator=(const request&);
	request(const request& other);

	const config			config_;
	timer					timer_;
	smtp&					socket_;
	bool					allow_;

public:
	request(const config& c, smtp& sc) : 
		config_(c), 
		socket_(sc),
		allow_(false)
	{}

	~request() {}

	bool operator() (const std::string& rcpt);

	bool allowed() const
	{
		return allow_;
	}

	bool denied() const
	{
		return !allow_;
	}
};
