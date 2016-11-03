// synch.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "util_synch.hpp"

namespace win32
{


class critical_section
{
private:
  // non-copyable and non-assignable
  critical_section(const critical_section&);
  critical_section& operator=(const critical_section&);

  CRITICAL_SECTION primitive_;

public:
  critical_section()
  {
    // TODO : handle potential SEH, documented at
    // http://msdn.microsoft.com/library/en-us/dllproc/base/initializecriticalsection.asp
    InitializeCriticalSection(&primitive_);
  }

  ~critical_section()
  {
    DeleteCriticalSection(&primitive_);
  }

private:
  void acquire()
  {
    // TODO : handle potential SEH, documented at
    // http://msdn.microsoft.com/library/en-us/dllproc/base/entercriticalsection.asp
    EnterCriticalSection(&primitive_);
  }

#if(_WIN32_WINNT >= 0x0400)
  bool try_acquire()
  {
    return TryEnterCriticalSection(&primitive_) != 0;
  }
#endif

  void release() 
  {
    LeaveCriticalSection(&primitive_);
  }

  friend class sync::lock<win32::critical_section>;
};


class mutex
{
private:
  // non-copyable and non-assignable
  mutex(const mutex&);
  mutex& operator=(const mutex&);

  HANDLE handle_;

public:
  mutex()
  {
    handle_ = CreateMutexA (NULL, FALSE, NULL);
    if (!handle_)
      throw sync::error("CreateMutex failed");
  }

  explicit mutex(const char* name, bool attach = false)
  {
    if (!attach)
    {
      handle_ = CreateMutexA(NULL, FALSE, name);
      if (!handle_)
        throw sync::error("CreateMutex failed");
    }
    else
    {
      handle_ = OpenMutexA(MUTEX_MODIFY_STATE | SYNCHRONIZE, FALSE, name);
      if (!handle_)
        throw sync::error("OpenMutex failed");
    }
  }

  explicit mutex(const wchar_t* name, bool attach = false)
  {
    if (!attach)
    {
      handle_ = CreateMutexW(NULL, FALSE, name);
      if (!handle_)
        throw sync::error("CreateMutex failed");
    }
    else
    {
      handle_ = OpenMutexW(MUTEX_MODIFY_STATE | SYNCHRONIZE, FALSE, name);
      if (!handle_)
        throw sync::error("OpenMutex failed");
    }
  }

  ~mutex()
  {
    CloseHandle(handle_);
  }

private:
  void acquire()
  {
    WaitForSingleObject(handle_, INFINITE);
  }

  bool try_acquire()
  {
    DWORD result = WaitForSingleObject(handle_, 0);
    return result != WAIT_TIMEOUT;
  }

  bool try_acquire(unsigned long timeout)
  {
    DWORD result = WaitForSingleObject(handle_, timeout);
    return result != WAIT_TIMEOUT;
  }

  void release()
  {
    ReleaseMutex(handle_);
  }

  friend class sync::lock<win32::mutex>;
};


class handle
{
	// non-copyable & non-assignable
	handle(const handle&);
	handle& operator=(const handle&);

	HANDLE handle_;

public:
	handle() : handle_(NULL) {}
	explicit handle(HANDLE handle) : handle_(handle) {}

	handle& operator=(HANDLE handle)
	{
		if (handle == handle_)
			return *this;
		if (handle_)
			CloseHandle(handle_);
		handle_ = handle;
		return *this;
	}

	~handle()
	{
		if (handle_)
			CloseHandle(handle_);
	}

	operator HANDLE* ()
	{
		return &handle_;
	}
};


} // namespace win32


