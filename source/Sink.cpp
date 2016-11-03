// Sink.cpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"
#include "Sink.h"

#include <MailMsgProps.h>

#include "request.hpp"

namespace
{

const unsigned int max_command = 1000;
const unsigned int max_response = 1050;
const unsigned int min_command = 8;

const unsigned int deny_status = 550;

const unsigned int max_message = 500;
const unsigned int max_ip = 16;

std::string read_rcpt(ISmtpInCommandContext *pContext)
{
	if (pContext == NULL)
		AtlThrow(E_POINTER);

	char buf[max_command];
	DWORD size = sizeof(buf);
	com::enforce(pContext->QueryCommand(buf, &size));

	if (size < min_command)
		throw CSink::error("Invalid SMTP protocol command");

	std::string keyword(buf, buf + 4);
	str::upper(keyword);
	if (keyword != "RCPT")
		throw CSink::error("SMTP protocol command different that RCPT");

	std::string result(buf + min_command);
	str::trim(result);
	str::lower(result);
	if (result.empty())
		throw CSink::error("Empty recipient address");

	if (result[0] == '<')
	{
		size_t close = result.find_first_of(">");
		if (close == std::string::npos)
			throw CSink::error("Invalid recipient address");
		else if (close >= 2)
			result = result.substr(1, close - 1);
		else
			result = "<>";
	}
	else if (result.find_first_of("<>") != std::string::npos)
		throw CSink::error("Invalid recipient address");

	return result;
}

unsigned long read_client_ip(IMailMsgProperties *pMsg)
{
	com::enforce(pMsg);

	char buf[max_ip] = {0};
	if (pMsg->GetStringA(IMMPID_MP_CONNECTION_IP_ADDRESS, sizeof(buf) - 1, buf) != S_OK)
		return tcp::ip4_none; // Mail from pickup directory

	unsigned long result = tcp::ip4_addr(buf);
	if (result == tcp::ip4_none)
		throw CSink::error("Invalid client IP address");

	return result;
}

void deny(ISmtpInCommandContext *pContext, bool disconnect, const std::string& response, unsigned int status)
{
	if (pContext == NULL)
		AtlThrow(E_POINTER);

	// calling C API from C++ is sometimes ugly. I promise that SetResponse won't try to change its first argument
	com::enforce(pContext->SetResponse(const_cast<LPSTR> (response.c_str()), static_cast<DWORD> (response.size())));
	com::enforce(pContext->SetProtocolErrorFlag(TRUE));
	
	DWORD result = EXPE_COMPLETE_FAILURE;
	if (!disconnect && status < 500)
		result = EXPE_TRANSIENT_FAILURE;
	else if (disconnect)
		result = EXPE_DROP_SESSION;

	com::enforce(pContext->SetCommandStatus(result));
	com::enforce(pContext->SetSmtpStatusCode(status));
}

HRESULT exception_handler(const char* function)
{
	char message[max_message] = {0};
	try
	{
		throw;
	}
	catch(const CAtlException& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, CAtlException : 0x%.08X\n", function, e.m_hr))
			OutputDebugStringA(message);
		return E_UNEXPECTED;
	}
	catch (const config::error& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, config::error : %s\n", function, e.what()))
			OutputDebugStringA(message);
		return S_OK;
	}
	catch (const tcp::error& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, tcp::error : %s\n", function, e.what()))
			OutputDebugStringA(message);
		return S_OK;
	}
	catch (const CSink::error& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, CSink::error : %s\n", function, e.what()))
			OutputDebugStringA(message);
		return S_OK;
	}
	catch (const std::exception& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, %s : %s\n", function, typeid(e).name(), e.what()))
			OutputDebugStringA(message);
		return E_UNEXPECTED;
	}
}

} // unnamed namespace

STDMETHODIMP CSink::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISink
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CSink::OnSmtpInCommand(IUnknown *pServer, IUnknown *pSession, IMailMsgProperties *pMsg, ISmtpInCommandContext *pContext)
{
	HRESULT result =  S_OK;

	try
	{
		{
			const sync::scoped_lock& g_metabase = sync::acquire(metabase_);
			if (metabase_.get() == NULL)
			{
				const sync::scoped_lock& g_mbpath = sync::acquire(mbpath_);
				if (mbpath_.get() == NULL)
					throw CSink::error("Metabase path is not set");
				metabase_.reset(new metabase(*mbpath_));
			} // free mbpath_ lock

			config c(*metabase_);
			{
				const sync::scoped_lock& g_sockets = sync::acquire(socket_);
				if (c.refresh || socket_.get() == NULL || !socket_->is_alive(c))
				{
					socket_.reset(); // Delete must be executed first
					socket_.reset(new smtp(config(*metabase_, config::complete)));
				}
				else
					socket_->reset_timer(c.request_max_delay);
			} // free socket_ lock
		} // free metabase_ lock

		// configuration is lock-free
		const config& c = socket_->configuration;

		unsigned long client_ip = read_client_ip(pMsg);
		if (client_ip == tcp::ip4_none || c.is_excluded(client_ip))
			return result;

		std::string rcpt = read_rcpt(pContext);
		request r(c, *socket_);

		if (r(rcpt))
		{
			if (r.denied())
			{
				std::string response(static_cast<std::string::size_type>(4), ' ');
				response[0] = char((c.rcpt_status % 1000) / 100) + '0';
				response[1] = char((c.rcpt_status % 100) / 10) + '0';
				response[2] = char(c.rcpt_status % 10) + '0';
				response += c.rcpt_response;
				if (c.rcpt_append)
					response += rcpt;
				response += "\r\n";

				deny(pContext, c.force_disconnect, response, c.rcpt_status);
				result = S_FALSE;
			}
		}
	}
	catch(...)
	{
		result = exception_handler(__FUNCTION__);
	}

	return result;
}

