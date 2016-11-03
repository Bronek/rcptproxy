// Sink2.cpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"
#include "Sink.h"

#include "util.hpp"
#include "metabase.hpp"

namespace
{
const wchar_t*			ninstance = L"Instance";
const wchar_t*			nbinding = L"Binding";
// "Configuration" also defined in metabase.cpp
const wchar_t*			nconfig = L"Configuration";

// "RcptProxy.Sink" also defined in sink.rgs
const wchar_t*			sink_class = L"RcptProxy.Sink";
const wchar_t*			sink_name = L"RcptProxy SMTP Sink";

const wchar_t*			nrule = L"Rule";
const wchar_t*			rule = L"RCPT";
const wchar_t*			npriority = L"Priority";

const unsigned int		max_message = 500;

// also defined in config.cpp
const unsigned int		sexcl = 0x00010018; // 65560
const unsigned int		srefr = 1;

// maximum count of wchar_t in exclusion list containing 2 IP addresses. Quite generous
const unsigned int		sexcl_buffer = 50;


template <typename Type>
void read(Type& dest, const wchar_t* key, IPropertyBag* bag, IErrorLog * log)
{
	CComVariant var;
	com::enforce(bag->Read(CComBSTR(key), &var, log));

	if (!com::cast(dest, var))
		throw CSink::error("Unable to read configuration from IPropertyBag");
}

HRESULT exception_handler(const char* function)
{
	char message[max_message];

	try
	{
		throw;
	}
	catch(const CAtlException& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, CAtlException : 0x%.08X\n", function, e.m_hr))
			OutputDebugStringA(message);
		return e.m_hr;
	}
	catch (const CSink::error& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, CSink::error : %s\n", function, e.what()))
			OutputDebugStringA(message);
		return E_FAIL;
	}
	catch (const std::exception& e)
	{
		if (str::format(std::nothrow, message, "Exception in %s, %s : %s\n", function, typeid(e).name(), e.what()))
			OutputDebugStringA(message);
		return E_UNEXPECTED;
	}
}

} // unnamed namespace

STDMETHODIMP CSink::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
	HRESULT result = E_NOTIMPL;

	try
	{
		if (pPropBag == NULL)
			AtlThrow(E_POINTER);

		const sync::scoped_lock& g = sync::acquire(mbpath_);

		if (mbpath_.get() != NULL)
			return S_OK;

		unsigned int instance;
		read(instance, ninstance, pPropBag, pErrorLog);

		std::wstring binding;
		read(binding, nbinding, pPropBag, pErrorLog);

		mbpath_.reset(new metabase::path(binding, instance));

		result = S_OK;
	}
	catch(...)
	{
		result = exception_handler(__FUNCTION__);
	}

	return result;
}

STDMETHODIMP CSink::Register(VARIANT instance, BSTR binding_guid, VARIANT_BOOL enabled, VARIANT priority, BSTR server_address)
{
	HRESULT result = S_OK;

	try
	{
		com::enforce(static_cast<void *> (server_address));

		if (tcp::ip4_addr(static_cast<const wchar_t*> (server_address)) == tcp::ip4_none)
		{
			AtlReportError(CLSID_Sink, "Invalid IP address", IID_ISink, E_FAIL);
			AtlThrow(E_FAIL);
		}

		CComPtr<IEventUtil> eutil;
		com::enforce(eutil.CoCreateInstance(__uuidof(CEventUtil), NULL, CLSCTX_INPROC_SERVER));

		BSTR sourceGUID;
		CComVariant vinstance(instance);
		com::enforce(vinstance.ChangeType(VT_UI4));
		com::enforce(eutil->GetIndexedGUID(CComBSTR(g_szGuidSmtpSvcSource), vinstance.ulVal, &sourceGUID));
		eutil.Release();

		CComPtr<IEventManager> emgr;
		com::enforce(emgr.CoCreateInstance(__uuidof(CEventManager), NULL, CLSCTX_INPROC_SERVER));

		CComPtr<IEventSourceTypes> srctypes;
		com::enforce(emgr->get_SourceTypes(&srctypes));

		CComPtr<IEventSourceType> srctype;
		CComVariant vsrctype = (g_szGuidSmtpSourceType);
		com::enforce(srctypes->Item(&vsrctype, &srctype));
		com::enforce(srctype);

		CComPtr<IEventSources> srcs;
		com::enforce(srctype->get_Sources(&srcs));

		CComPtr<IEventSource> src;
		CComVariant vsrc(sourceGUID);
		com::enforce(srcs->Item(&vsrc, &src));
		com::enforce(src);

		CComPtr<IEventBindingManager> bmgr;
		com::enforce(src->GetBindingManager(&bmgr));

		CComPtr<IEventBindings> bindings;
		com::enforce(bmgr->get_Bindings(CComBSTR(g_szcatidSmtpOnInboundCommand), &bindings));

		CComPtr<IEventBinding> binding;
		com::enforce(bindings->Add(binding_guid, &binding));
		
		com::enforce(binding->put_SinkClass(CComBSTR(sink_class)));
		com::enforce(binding->put_DisplayName(CComBSTR(sink_name)));
		com::enforce(binding->put_Enabled(enabled));

		CComPtr<IEventPropertyBag> source;
		com::enforce(binding->get_SourceProperties(&source));
		
		CComVariant vrcpt(rule);
		com::enforce(source->Add(CComBSTR(nrule), &vrcpt));

		CComVariant vpriority(priority);
		com::enforce(vpriority.ChangeType(VT_UI2));
		com::enforce(source->Add(CComBSTR(npriority), &vpriority));

		source.Release();

		CComPtr<IEventPropertyBag> props;
		com::enforce(binding->get_SinkProperties(&props));
		
		com::enforce(props->Add(CComBSTR(ninstance), &vinstance));

		CComVariant vbinding(binding_guid);
		com::enforce(props->Add(CComBSTR(nbinding), &vbinding));

		CComVariant vconfig(server_address);
		com::enforce(props->Add(CComBSTR(nconfig), &vconfig));

		com::enforce(binding->Save());

		const sync::scoped_lock& g_mbpath = sync::acquire(mbpath_);
		mbpath_.reset(new metabase::path(static_cast<const wchar_t*> (vbinding.bstrVal), vinstance.ulVal));

		init();
	}
	catch(...)
	{
		result = exception_handler(__FUNCTION__);
	}

	return result;
}

STDMETHODIMP CSink::Unregister(VARIANT instance, BSTR binding_guid)
{
	HRESULT result = S_OK;

	try
	{
		CComPtr<IEventUtil> eutil;
		com::enforce(eutil.CoCreateInstance(__uuidof(CEventUtil), NULL, CLSCTX_INPROC_SERVER));

		BSTR sourceGUID;
		CComVariant vinstance(instance);
		com::enforce(vinstance.ChangeType(VT_UI4));
		com::enforce(eutil->GetIndexedGUID(CComBSTR(g_szGuidSmtpSvcSource), vinstance.ulVal, &sourceGUID));
		eutil.Release();

		CComPtr<IEventManager> emgr;
		com::enforce(emgr.CoCreateInstance(__uuidof(CEventManager), NULL, CLSCTX_INPROC_SERVER));

		CComPtr<IEventSourceTypes> srctypes;
		com::enforce(emgr->get_SourceTypes(&srctypes));

		CComPtr<IEventSourceType> srctype;
		CComVariant vsrctype = (g_szGuidSmtpSourceType);
		com::enforce(srctypes->Item(&vsrctype, &srctype));
		com::enforce(srctype);

		CComPtr<IEventSources> srcs;
		com::enforce(srctype->get_Sources(&srcs));

		CComPtr<IEventSource> src;
		CComVariant vsrc(sourceGUID);
		com::enforce(srcs->Item(&vsrc, &src));
		com::enforce(src);

		CComPtr<IEventBindingManager> bmgr;
		com::enforce(src->GetBindingManager(&bmgr));

		CComPtr<IEventBindings> bindings;
		com::enforce(bmgr->get_Bindings(CComBSTR(g_szcatidSmtpOnInboundCommand), &bindings));

		CComPtr<IEventBinding> binding;
		CComVariant vbinding_guid(binding_guid);
		com::enforce(bindings->Remove(&vbinding_guid));
	}
	catch(...)
	{
		result = exception_handler(__FUNCTION__);
	}

	return result;
}


void CSink::init()
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

	std::wstring exclusions = L"127.0.0.1";
	exclusions += std::wstring(L"\0", 1);
	if (c.server_address != tcp::ip4_addr(exclusions.c_str()))
	{
		std::wstring tmp;
		str::format(tmp, L"%u.%u.%u.%u", c.server_address & 0xFF, (c.server_address & 0xFF00) >> 8, (c.server_address & 0xFF0000) >> 16, (c.server_address & 0xFF000000) >> 24);
		tmp += std::wstring(L"\0", 1);
		exclusions += tmp;
	}
	exclusions += std::wstring(L"\0", 1);

	unsigned char buf[sexcl_buffer * sizeof(wchar_t)];
	exclusions.copy(reinterpret_cast<wchar_t*> (buf), sexcl_buffer);

	METADATA_RECORD record = {sexcl, 0, 0, MULTISZ_METADATA, static_cast<DWORD> (exclusions.size() * sizeof(wchar_t)), buf, 0};
	metabase_->write(record);

	unsigned char buf2[sizeof(DWORD)] = {0};
	METADATA_RECORD record2 = {srefr, 0, 0, DWORD_METADATA, sizeof(buf2), buf2, 0};
	metabase_->write(record2);
}
