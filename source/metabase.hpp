// metabase.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "util.hpp"
#include "util_synch.hpp"
#include "util_win32.hpp"

class metabase
{
public:
	class path
	{
		const std::wstring		binding_;

	public:
		// read-only primitive types - safe for mutithreading without locks
		const unsigned int		instance;
		const wchar_t* const	binding;

		path(const std::wstring& bind, unsigned int instance) :
			binding_(bind),
			instance(instance),
			binding(binding_.c_str())
		{}

		path(const path& other) :
			binding_(other.binding),
			instance(other.instance),
			binding(binding_.c_str())
		{}
	};

private:
	const std::wstring					mbpath_;
	CComPtr<IMSAdminBase>				metabase_;
	win32::critical_section				lock_;

	// non-copyable and non-assignable
	metabase(const metabase&);
	metabase& operator=(const metabase&);

public:
	metabase(const metabase::path& path);

	~metabase()
	{}

	HRESULT read(METADATA_RECORD& record, DWORD& size, const std::nothrow_t&);

	void read(METADATA_RECORD& record, DWORD& size);

	HRESULT write(METADATA_RECORD& record, const std::nothrow_t&);

	void write(METADATA_RECORD& record);
};

