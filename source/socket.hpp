// socket.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and
// distribution is subject to the Common Public License Version 1.0
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "util.hpp"
#include "timer.hpp"

namespace tcp
{

enum {ip4_none = INADDR_NONE};

// This is to isolate other classes from inet_addr winsock function ...
inline unsigned long ip4_addr(const char* sz)
{
	unsigned int q1 = 0;
	unsigned int q2 = 0;
	unsigned int q3 = 0;
	unsigned int q4 = 0;
	if (sscanf(sz, "%u.%u.%u.%u", &q1, &q2, &q3, &q4) != 4 ||
		(q1 & 0xFFFFFF00) || (q2 & 0xFFFFFF00) || (q3 & 0xFFFFFF00) || (q4 & 0xFFFFFF00))
		return tcp::ip4_none;

	return (q4 << 24) + (q3 << 16) + (q2 << 8) + q1;
}

// ... and to provide some extra functionality
inline unsigned long ip4_addr(const wchar_t* wsz)
{
	unsigned int q1 = 0;
	unsigned int q2 = 0;
	unsigned int q3 = 0;
	unsigned int q4 = 0;
	if (swscanf(wsz, L"%u.%u.%u.%u", &q1, &q2, &q3, &q4) != 4 ||
		(q1 & 0xFFFFFF00) || (q2 & 0xFFFFFF00) || (q3 & 0xFFFFFF00) || (q4 & 0xFFFFFF00))
		return tcp::ip4_none;

	return (q4 << 24) + (q3 << 16) + (q2 << 8) + q1;
}

class error : public std::runtime_error
{
private:
	static const unsigned int max_message = 500;

public:
	explicit error(const char* msg) : std::runtime_error(msg) {}

	// Winsock2 error codes are documented in MSDN
	// http://msdn.microsoft.com/library/en-us/winsock/winsock/windows_sockets_error_codes_2.asp
	// ms-help://MS.VSCC.2003/MS.MSDNQTR.2004JUL.1033/winsock/winsock/windows_sockets_error_codes_2.htm
	// http://support.microsoft.com/?kbid=819124
	static void last(const char* function)
	{
		int err = WSAGetLastError();
		if (err == WSA_IO_PENDING || err == WSA_IO_INCOMPLETE)
			return; // not an error

		char message[max_message];
		if (str::format(std::nothrow, message, "%s returned error %d", function, err))
			throw error(message);
		else
			throw error("Winsock returned some error");
	}
};

class ip4_host
{
	unsigned long ip_;
	unsigned short port_;

public:
	ip4_host(unsigned long ip, unsigned short port) :
		ip_(ip),
		port_(port)
	{
		if (this->ip_ == tcp::ip4_none)
			throw error("Bad ip4_host address");
	}

	ip4_host(const char* ip, unsigned short port) :
		ip_(ip4_addr(ip)),
		port_(port)
	{
		if (this->ip_ == tcp::ip4_none)
			throw error("Bad ip4_host address");
	}

	unsigned long ip() const
	{
		return ip_;
	}

	unsigned short port() const
	{
		return port_;
	}

	friend bool operator== (const ip4_host& lh, const ip4_host& rh)
	{
		return lh.ip_ == rh.ip_ && lh.port_ == rh.port_;
	}

	friend bool operator!= (const ip4_host& lh, const ip4_host& rh)
	{
		return !(lh == rh);
	}
};

template <unsigned int Size>
class event_array
{
	// non-copyable & non-assignable
	event_array& operator= (const event_array&);
	event_array(const event_array&);

	WSAEVENT events_[Size];

public:
	static const unsigned int size = Size;

	event_array()
	{
		unsigned int i = 0;
		try
		{
			for (i = 0; i < Size; ++i)
			{
				events_[i] = WSACreateEvent();
				if (events_[i] == WSA_INVALID_EVENT)
					error::last("WSACreateEvent");
			}
		}
		catch(tcp::error&)
		{
			// we cannot close events before throwing exception,
			// because that might skew error state of Winsock
			for (unsigned int j = 0; j < i; ++j)
				WSACloseEvent(events_[j]);
			throw;
		}
	}

	event_array(const std::nothrow_t&)
	{
		// to be called where exception is inapropriate (eg. disc called in destructor)
		// caller must check state of all event objects manually
		for (unsigned int i = 0; i < Size; ++i)
		{
			events_[i] = WSACreateEvent();
			if (events_[i] == WSA_INVALID_EVENT)
				return;
		}
	}

	~event_array()
	{
		for (unsigned int i = 0; i < Size; ++i)
		{
			if (events_[i] != WSA_INVALID_EVENT)
				WSACloseEvent(events_[i]);
			else
				return;
		}
	}

	operator WSAEVENT* ()
	{
		return events_;
	}
};

template <typename Protocol>
class socket
{
	// non-copyable and non-assignable
	socket(const socket&);
	socket& operator=(const socket&);

	SOCKET						socket_;
	util::array<char>			buffer_;
	ip4_host					server_;
	bool						connected_;

	enum {overlapped_sleep_ = 60};

protected:
	explicit socket(const ip4_host& s) :
		buffer_(Protocol::buffer_size_),
		server_(s),
		connected_ (false)
	{
		socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (socket_ == INVALID_SOCKET)
			error::last("WSASocket");

		sockaddr_in saddr = {AF_INET, htons(s.port())};
		saddr.sin_addr.s_addr = s.ip();

		if (connect(socket_, reinterpret_cast<SOCKADDR*> (&saddr), sizeof(saddr)) == SOCKET_ERROR)
			error::last("connect");


		connected_ = true;
	}

	virtual ~socket()
	{
		disc("");
	}

	// safe for call inside destructor
	void disc(const std::string& data)
	{
		if (!connected_)
			return;

		event_array<1> events(std::nothrow);
		WSAOVERLAPPED overlapped;

		if (Protocol::send_bye_command_ && !data.empty() && events[0] != WSA_INVALID_EVENT)
		{
			timer t;
			// I promise that this data won't be modified ! It's just
			// than Winsock don't let me pass const buffer
			char* datac = const_cast<char *> (data.c_str());

			unsigned int sent = 0;
			do
			{
				WSABUF buffer = {static_cast<unsigned int> (data.size()) - sent, &datac[sent]};
				overlapped.Internal = overlapped.InternalHigh = overlapped.Offset = overlapped.OffsetHigh = 0;
				overlapped.hEvent = events[0];

				DWORD bytes = 0;
				if (WSASend(socket_, &buffer, 1, &bytes, 0, &overlapped, NULL) == SOCKET_ERROR
					&& WSAGetLastError() != WSA_IO_PENDING)
					break;

				if (WSAWaitForMultipleEvents(events.size, events, FALSE, static_cast<unsigned long> (t.ms(Protocol::close_timeout_)), FALSE) == WSA_WAIT_FAILED)
					break;

				if (!WSAResetEvent(events[0]))
					break;

				DWORD flags = 0;
				if (!WSAGetOverlappedResult(socket_, &overlapped, &bytes, TRUE, &flags))
					break;

				if (!bytes)
					break;

				sent += bytes;

				Sleep(overlapped_sleep_);
			} while (sent < data.length() && t.ms(Protocol::close_timeout_) > 0LL);
		}

		// cancel pending IO operation, if there is any
		CancelIo(reinterpret_cast<HANDLE> (socket_));
		WSAResetEvent(events[0]);

		if (Protocol::close_gracefully_ && events[0] != WSA_INVALID_EVENT)
		{
			timer t;
			// gracefull disconnect
			shutdown(socket_, SD_SEND);
			DWORD bytes = 0;
			do
			{
				WSABUF buffer = {static_cast<unsigned int> (buffer_.size), buffer_};
				overlapped.Internal = overlapped.InternalHigh = overlapped.Offset = overlapped.OffsetHigh = 0;
				overlapped.hEvent = events[0];

				DWORD flags = 0;

				if (WSARecv(socket_, &buffer, 1, &bytes, &flags, &overlapped, NULL) == SOCKET_ERROR
					&& WSAGetLastError() != WSA_IO_PENDING)
					break;

				if (WSAWaitForMultipleEvents(events.size, events, FALSE, static_cast<unsigned long> (t.ms(Protocol::close_timeout_)), FALSE) == WSA_WAIT_FAILED)
					break;

				if (!WSAResetEvent(events[0]))
					break;

				if (!WSAGetOverlappedResult(socket_, &overlapped, &bytes, FALSE, &flags))
					break;

				Sleep(overlapped_sleep_);
			} while (bytes && t.ms(Protocol::close_timeout_) > 0LL);
		}

		closesocket(socket_);
		connected_ = false;
	}

	void recv(const timer& t, unsigned long m)
	{
		event_array<1> events;
		WSAOVERLAPPED overlapped;

		DWORD bytes = 0;
		do
		{
			if (t.ms(m) == 0)
				throw error("Timeout expired in socket::recv");

			WSABUF buffer = {static_cast<unsigned int> (buffer_.size), buffer_};
			overlapped.Internal = overlapped.InternalHigh = overlapped.Offset = overlapped.OffsetHigh = 0;
			overlapped.hEvent = events[0];

			DWORD flags = 0;

			if (WSARecv(socket_, &buffer, 1, &bytes, &flags, &overlapped, NULL) == SOCKET_ERROR
				&& WSAGetLastError() != WSA_IO_PENDING)
				error::last("WSARecv in socket::recv");

			if (WSAWaitForMultipleEvents(events.size, events, FALSE, static_cast<unsigned long> (t.ms(m)), FALSE) == WSA_WAIT_FAILED)
				error::last("WSAWaitForMultipleEvents in socket::recv");

			if (!WSAResetEvent(events[0]))
				error::last("WSAResetEvent in socket::recv");

			while (!WSAGetOverlappedResult(socket_, &overlapped, &bytes, FALSE, &flags))
			{
				error::last("WSAGetOverlappedResult in socket::recv");

				if (t.ms(m) == 0)
					throw error("Timeout expired in socket::recv");

				Sleep(overlapped_sleep_);
			}

			if (!(static_cast<Protocol*>(this))->on_recv(buffer_, bytes))
				break;

			Sleep(overlapped_sleep_);
		} while (true);
	}

	void send(const std::string& data, const timer& t, unsigned long m)
	{
		// I promise that this data won't be modified ! It's just
		// than Winsock don't let me pass const buffer
		char* datac = const_cast<char *> (data.c_str());
		event_array<1> events;
		WSAOVERLAPPED overlapped;

		unsigned int sent = 0;
		do
		{
			if (t.ms(m) == 0)
				throw error("Timeout expired in socket::send");

			WSABUF buffer = {static_cast<unsigned int> (data.size()) - sent, &datac[sent]};
			overlapped.Internal = overlapped.InternalHigh = overlapped.Offset = overlapped.OffsetHigh = 0;
			overlapped.hEvent = events[0];

			DWORD bytes = 0;
			if (WSASend(socket_, &buffer, 1, &bytes, 0, &overlapped, NULL) == SOCKET_ERROR
				&& WSAGetLastError() != WSA_IO_PENDING)
				error::last("WSARecv in socket::send");

			if (WSAWaitForMultipleEvents(events.size, events, FALSE, static_cast<unsigned long> (t.ms(m)), FALSE) == WSA_WAIT_FAILED)
				error::last("WSAWaitForMultipleEvents in socket::send");

			if (!WSAResetEvent(events[0]))
				error::last("WSAResetEvent in socket::send");

			DWORD flags = 0;

			if (!WSAGetOverlappedResult(socket_, &overlapped, &bytes, TRUE, &flags))
				error::last("WSAGetOverlappedResult in socket::send");

			if (!bytes)
				throw error("Connection broken in socket::send");

			sent += bytes;
			if (sent >= data.length())
				break;

			Sleep(overlapped_sleep_);
		} while (true);
	}

public:
	bool is_alive(const ip4_host& s)
	{
		if (!connected_)
			return false;
		else if (server_ != s)
			return false;

		return (static_cast<Protocol*>(this))->on_is_alive(s);
	}
};

} // namespace tcp
