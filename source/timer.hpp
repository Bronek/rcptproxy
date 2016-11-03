// timer.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and
// distribution is subject to the Common Public License Version 1.0
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

class timer
{
	__int64 start_;
public:
	const __int64 frequency;
	const __int64& started;

	static __int64 now()
	{
		LARGE_INTEGER perf;
		QueryPerformanceCounter(&perf);
		return perf.QuadPart;
	}

	static __int64 freq()
	{
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		return perf.QuadPart;
	}

	timer() :
		start_(now()),
		frequency(freq()),
		started(start_)
	{}

	void reset()
	{
		start_ = now();
	}

	__int64 s() const
	{
    // seconds
		return (now() - start_) / frequency;
	}

	__int64 ms() const
	{
    // milliseconds
		return (1000LL * (now() - start_)) / frequency;
	}

	__int64 us() const
	{
    // microseconds
		return (1000000LL * (now() - start_)) / frequency;
	}

	__int64 ns() const
	{
    // nanoseconds
		return (1000000000LL * (now() - start_)) / frequency;
	}

	__int64 s(__int64 t) const
	{
    // remaining seconds
		__int64 n = s();
		if (n > t)
			return 0;
		else
			return t - n;
	}

	__int64 ms(__int64 t) const
	{
    // remaining milliseconds
		__int64 n = ms();
		if (n > t)
			return 0;
		else
			return t - n;
	}

	__int64 us(__int64 t) const
	{
    // remaining microseconds
		__int64 n = us();
		if (n > t)
			return 0;
		else
			return t - n;
	}

	__int64 ns(__int64 t) const
	{
    // remaining nanoseconds
		__int64 n = ns();
		if (n > t)
			return 0;
		else
			return t - n;
	}
};


