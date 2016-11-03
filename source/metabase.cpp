// metabase.cpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#include "stdafx.h"

#include "metabase.hpp"

namespace
{

const int max_binding_string = 1000;

// used only inside mbpath2wstring() ; "Configuration" also defined in sink2.cpp
const wchar_t* const mbpath_format = L"/LM/SmtpSvc/%d/EventManager/EventTypes/%s/Bindings/%s/SinkProperties/Configuration";
const wchar_t* const event_type = g_szcatidSmtpOnInboundCommand;

std::wstring mbpath2wstring(const metabase::path& path)
{
	wchar_t binding[max_binding_string];
	str::format(binding, mbpath_format, path.instance, event_type, path.binding);
	return std::wstring(binding);
}

} // unnamed namespace

metabase::metabase(const metabase::path& path) : mbpath_(mbpath2wstring(path))
{
	com::enforce(metabase_.CoCreateInstance(CLSID_MSAdminBase, 0, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER));
}

HRESULT metabase::read(METADATA_RECORD& record, DWORD &size, const std::nothrow_t&)
{
	const sync::scoped_lock& g = sync::acquire(lock_);
	size = 0;
	HRESULT hr = metabase_->GetData(METADATA_MASTER_ROOT_HANDLE, mbpath_.c_str(), &record, &size);
	return hr;
}

void metabase::read(METADATA_RECORD& record, DWORD &size)
{
	com::enforce(this->read(record, size, std::nothrow));
}

HRESULT metabase::write(METADATA_RECORD& record, const std::nothrow_t&)
{
	const sync::scoped_lock& g = sync::acquire(lock_);

	METADATA_HANDLE handle;
	HRESULT hr = metabase_->OpenKey(METADATA_MASTER_ROOT_HANDLE, mbpath_.c_str(), METADATA_PERMISSION_WRITE, INFINITE, &handle);
	if (FAILED(hr))
		return hr;

	hr = metabase_->SetData(handle, L"", &record);
	metabase_->CloseKey(handle);
	return hr;
}

void metabase::write(METADATA_RECORD& record)
{
	com::enforce(this->write(record, std::nothrow));
}
