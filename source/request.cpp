// request.cpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"

#include "request.hpp"

bool request::operator() (const std::string& rcpt)
{
	if (rcpt.empty())
		return false;

	std::string from(config_.protocol_from);
	if (from.empty())
		return false;

	std::string cmd = "MAIL FROM: ";
	if (from[0] != '<')
	{
		cmd += "<";
		cmd += from;
		cmd += ">";
	}
	else
		cmd += from;
	cmd += "\r\n";

	const sync::scoped_lock& g = sync::acquire(socket_);

	if (socket_.send_recv(cmd) >= 300)
	{
		socket_.disc();
		return false;
	}

	cmd = "RCPT TO: ";
	if (rcpt[0] != '<')
	{
		cmd += "<";
		cmd += rcpt;
		cmd += ">";
	}
	else
		cmd += rcpt;
	cmd += "\r\n";

	allow_ = (socket_.send_recv(cmd) < 300);
	return true;
}

