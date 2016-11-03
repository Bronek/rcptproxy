// config.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"

#include "sink.h"

#include "util.hpp"
#include "config.hpp"

namespace
{

const unsigned int		saddr = 0;

const unsigned int		srefr = 1;

const unsigned int		sport = 0x00010011; // 65553
const unsigned short	dport = 25;

const unsigned int		shelo = 0x00010012; // 65554
const char* const		dhelo = "world";

const unsigned int		sidle = 0x00010013; // 65555
const unsigned int		didle = 300;

const unsigned int		smaxt = 0x00010014; // 65556
const unsigned int		dmaxt = 1800;

const unsigned int		sdisc = 0x00010015; // 65557
const bool				ddisc = true;

const unsigned int		sfrom = 0x00010016; // 65558
const char* const		dfrom = "rcptproxy@localhost";

const unsigned int		sdely = 0x00010017; // 65559
const unsigned int		ddely = 10000;

const unsigned int		sexcl = 0x00010018; // 65560

const unsigned int		sfrcp = 0x00010019; // 65561
const char* const		dfrcp = "Unable to relay to @"; // ending space means "append email here"

const unsigned int		sfsts = 0x0001001A; // 65562
const unsigned int		dfsts = 550;

const unsigned int		max_string = 80;
const unsigned int		sexcl_buffer = 800;
const unsigned int		sexcl_size = 60;
const unsigned int		max_ip = 16;

bool read(unsigned int& dest, metabase& md, unsigned int id)
{
	unsigned char buf[sizeof(DWORD)];
	METADATA_RECORD record = {id, 0, 0, DWORD_METADATA, sizeof(buf), buf, 0};
	DWORD size;
	if (md.read(record, size, std::nothrow) == S_OK)
	{
		dest = *(reinterpret_cast<DWORD *> (record.pbMDData));
		return true;
	}
	return false;
}

bool read(unsigned short& dest, metabase& md, unsigned int id)
{
	unsigned int tmp = 0;
	if (!read(tmp, md, id))
		return false;
	dest = static_cast<unsigned short> (tmp);
	return true;
}

bool read(bool& dest, metabase& md, unsigned int id)
{
	unsigned int tmp = 0;
	if (!read(tmp, md, id))
		return false;
	dest = (tmp != 0);
	return true;
}

bool read(std::string& dest, metabase& md, unsigned int id)
{
	unsigned char buf[max_string * sizeof(wchar_t)];
	METADATA_RECORD record = {id, 0, 0, STRING_METADATA, sizeof(buf), buf, 0};
	DWORD size;
	if (md.read(record, size, std::nothrow) == S_OK)
	{
		char dest_buf[max_string];
		int len = (record.dwMDDataLen / sizeof(wchar_t)) - 1;
		if (str::cast(dest_buf, reinterpret_cast<wchar_t*>(buf), len))
		{
			dest.assign(dest_buf, len);
			return true;
		}
	}

	return false;
}

template <typename Type>
Type read(metabase& md, unsigned int id)
{
	Type result;
	if (read(result, md, id))
		return result;

	throw config::error("Error reading metabase");
}

template <typename Type>
Type read(metabase& md, unsigned int id, const Type& def)
{
	Type result;
	if (read(result, md, id))
		return result;

	return def;
}

} // unnamed namespace

namespace
{
	bool message_append(std::string& msg)
	{
		if (msg.empty())
			return false;

		std::string::iterator i = --msg.end();
		if (*i == '@')
		{
			msg.erase(i, msg.end());
			return true;
		}

		return false;
	}
} // unnamed namespace

const config::complete_t config::complete;

void config::read_exclusions(metabase& md)
{
	unsigned char buf[sexcl_buffer * sizeof(wchar_t)];
	METADATA_RECORD record = {sexcl, 0, 0, MULTISZ_METADATA, sizeof(buf), buf, 0};
	DWORD size;
	HRESULT hr = md.read(record, size, std::nothrow);
	if (hr == S_OK)
		size = 0;
	else if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
		return;

	const wchar_t* wsz = reinterpret_cast<const wchar_t*> (&buf[0]);
	util::array<unsigned char> buf2(size);
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		record.dwMDDataLen = static_cast<DWORD> (buf2.size);
		record.pbMDData = buf2;
		if (md.read(record, size, std::nothrow) != S_OK)
			return;

		wsz = reinterpret_cast<const wchar_t*> (record.pbMDData);
	}

	const sync::scoped_lock& g = sync::acquire(exc_lock_);
	exclusions_.reserve(sexcl_size);
	while (*wsz!= 0)
	{
		unsigned long ip = tcp::ip4_addr(wsz);
		if (ip != tcp::ip4_none)
			exclusions_.push_back(ip);

		while (*wsz != 0)
			++wsz;
		++wsz;
	}

	std::sort(exclusions_.begin(), exclusions_.end());
	std::vector<unsigned long>::iterator i = std::unique(exclusions_.begin(), exclusions_.end());
	if (i != exclusions_.end())
		exclusions_.erase(i);
}

config::config(metabase& mb) :
	protocol_helo_(dhelo),
	protocol_from_(dfrom),
	rcpt_response_(dfrcp),
	refresh(read<bool>(mb, srefr, false)),
	server_address(tcp::ip4_addr(read<std::string>(mb, saddr).c_str())),
	server_port(read<unsigned short>(mb, sport, dport)),
	protocol_helo(protocol_helo_.c_str()),
	conn_idle_timeout(didle),
	conn_max_time(dmaxt),
	force_disconnect(ddisc),
	protocol_from(protocol_from_.c_str()),
	request_max_delay(ddely),
	rcpt_append(message_append(rcpt_response_)),
	rcpt_response(rcpt_response_.c_str()),
	rcpt_status(dfsts)
{}

config::config(metabase& mb, const complete_t&) :
	protocol_helo_(read<std::string>(mb, shelo, dhelo)),
	protocol_from_(read<std::string>(mb, sfrom, dfrom)),
	rcpt_response_(read<std::string>(mb, sfrcp, dfrcp)),
	refresh(read<bool>(mb, srefr, false)),
	server_address(tcp::ip4_addr(read<std::string>(mb, saddr).c_str())),
	server_port(read<unsigned short>(mb, sport, dport)),
	protocol_helo(protocol_helo_.c_str()),
	conn_idle_timeout(read<unsigned int>(mb, sidle, didle)),
	conn_max_time(read<unsigned int>(mb, smaxt, dmaxt)),
	force_disconnect(read<bool>(mb, sdisc, ddisc)),
	protocol_from(protocol_from_.c_str()),
	request_max_delay(read<unsigned int>(mb, sdely, ddely)),
	rcpt_append(message_append(rcpt_response_)),
	rcpt_response(rcpt_response_.c_str()),
	rcpt_status(read<unsigned int>(mb, sfsts, dfsts))
{
	read_exclusions(mb);

	unsigned char buf[sizeof(DWORD)] = {0};
	METADATA_RECORD record = {srefr, 0, 0, DWORD_METADATA, sizeof(buf), buf, 0};
	mb.write(record);
}

