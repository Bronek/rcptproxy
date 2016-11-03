// Sink.h

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once
#include "resource.h"       // main symbols

#include "rcptproxy.h"

#include "util_synch.hpp"
#include "util_win32.hpp"
#include "util_ptr.hpp"
#include "metabase.hpp"
#include "smtp.hpp"

// CSink

class ATL_NO_VTABLE CSink : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSink, &CLSID_Sink>,
	public ISupportErrorInfo,
	public IDispatchImpl<ISink, &IID_ISink, &LIBID_rcptproxyLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IEventIsCacheable ,
	public IPersistPropertyBag ,
	public ISmtpInCommandSink
{
	sync::ptr<metabase, win32::critical_section>			metabase_;
	sync::ptr<metabase::path, win32::critical_section>		mbpath_;
	sync::ptr<smtp, win32::critical_section>				socket_;

	// non-copyable and non-assignable
	CSink(const& CSink);
	CSink& operator=(const& CSink);

public:
	struct error : public std::runtime_error
	{
		explicit error(const char* msg) : std::runtime_error(msg) {}
	};

	CSink()
	{
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_SINK)

	BEGIN_COM_MAP(CSink)
		COM_INTERFACE_ENTRY(ISink)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY(IEventIsCacheable)
		COM_INTERFACE_ENTRY(IPersistPropertyBag)
		COM_INTERFACE_ENTRY(ISmtpInCommandSink)
	END_COM_MAP()

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease() 
	{
		{
			const sync::scoped_lock& g = sync::acquire(metabase_);
			metabase_.reset();
		} // free metabase_ lock

		{
			const sync::scoped_lock& g = sync::acquire(mbpath_);
			mbpath_.reset();
		} // free mbpath_ lock

		const sync::scoped_lock& g = sync::acquire(socket_);
		socket_.reset();
	}

	void init();

public:
	STDMETHOD(Register)(VARIANT instance, BSTR binding_guid, VARIANT_BOOL enabled, VARIANT priority, BSTR server_address);
	STDMETHOD(Unregister)(VARIANT instance, BSTR binding_guid);

	// IEventIsCacheable Methods
public:
	STDMETHOD(IsCacheable)()
	{
		return S_OK;
	}

	// IPersistPropertyBag Methods
public:
	STDMETHOD(InitNew)()
	{
		return S_OK;
	}

	STDMETHOD(Load)(IPropertyBag * pPropBag, IErrorLog * pErrorLog);

	STDMETHOD(Save)(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		return S_OK;
	}

	// IPersist Methods
public:
	STDMETHOD(GetClassID)(GUID * pClassID)
	{
		if (pClassID == NULL)
			return E_POINTER;
		
		*pClassID = CLSID_Sink;
		return S_OK;
	}

	// ISmtpInCommandSink Methods
public:
	STDMETHOD(OnSmtpInCommand)(IUnknown *pServer, IUnknown *pSession, IMailMsgProperties *pMsg, ISmtpInCommandContext *pContext);
};

OBJECT_ENTRY_AUTO(__uuidof(Sink), CSink)
