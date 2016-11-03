// util_ptr.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

#include "util_synch.hpp"

namespace sync
{

template <typename Type, typename Synch>
class ptr
{
	// non-copyable and non-assignable
	ptr(const ptr&);
	ptr& operator=(const ptr&);

	Synch syn_;
	std::auto_ptr<Type>	ptr_;

	// synchronization interface. 
	template <typename Type1, typename Synch1> 
	friend lock<Synch1> acquire(ptr<Type1, Synch1>& synch);

	template <typename Type1, typename Synch1> 
	friend lock<Synch1> try_acquire(ptr<Type1, Synch1>&, unsigned long);

	template <typename Type1, typename Synch1> 
	friend lock<Synch1> try_acquire(ptr<Type1, Synch1>& synch);

public:
	ptr() {}
	explicit ptr(Type * p) : ptr_(p) {}

	~ptr()
	{
		lock<Synch> g = acquire(syn_);
		ptr_.reset();
	}

	// proxy to auto_ptr interface
	Type& operator*() {return *ptr_;}
	Type* operator->() const {return ptr_.get();}
	Type* get() const {return ptr_.get();}
	void reset(Type * p = 0) {ptr_.reset(p);}

	// no "release", sorry. You do not need it.
};


template <typename Type, typename Synch>
inline lock<Synch> acquire(ptr<Type, Synch>& synch)
{
	return lock<Synch>(synch.syn_);
}

template <typename Type, typename Synch>
inline lock<Synch> try_acquire(ptr<Type, Synch>& synch, unsigned long timeout)
{
	return lock<Synch>(synch.syn_, timeout);
}

template <typename Type, typename Synch>
inline lock<Synch> try_acquire(ptr<Type, Synch>& synch)
{
	return lock<Synch>(synch.syn_, lock<Synch>::dont_wait());
}


} // namespace sync
