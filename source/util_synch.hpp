// synch.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

namespace sync
{


struct error : public std::runtime_error
{
  explicit error(const char* msg) : std::runtime_error(msg) {}
};


class scoped_lock
{
  // non-assignable and non-copyable
  scoped_lock& operator=(const scoped_lock&);
  scoped_lock(const scoped_lock& rh);

protected:
  scoped_lock() {}

public:
  virtual ~scoped_lock() {}
  virtual void unlock() const = 0;
  virtual bool active() const = 0;
};


template <typename Synch>
class lock : public scoped_lock
{
  // non-assignable
  lock& operator=(const lock&);

  mutable Synch* synch_;

  Synch* release() const
  {
    Synch* t = synch_;
    synch_ = NULL;

    return t;
  }

public:
  struct dont_wait{};

  explicit lock(Synch& synch) : synch_(&synch)
  {
    synch_->acquire();
  }

  lock(Synch& synch, unsigned long timeout) : synch_(&synch)
  {
    if (!synch_->try_acquire(timeout))
      release();
  }

  lock(Synch& synch, const dont_wait&) : synch_(&synch)
  {
    if (!synch_->try_acquire())
      release();
  }

  lock(const lock& rh) : scoped_lock(), synch_(rh.release())
  {
  }

  ~lock()
  {
    unlock();
  }

  void unlock() const
  {
    if (synch_ != NULL)
    {
      synch_->release();
      release();
    }
  }

  bool active() const
  {
    return synch_ != NULL;
  }
};


template <typename Synch>
inline lock<Synch> acquire(Synch& synch)
{
  return lock<Synch>(synch);
}


template <typename Synch>
inline lock<Synch> try_acquire(Synch& synch, unsigned long timeout)
{
  return lock<Synch>(synch, timeout);
}


template <typename Synch>
inline lock<Synch> try_acquire(Synch& synch)
{
  return lock<Synch>(synch, typename lock<Synch>::dont_wait());
}


} // namespace sync

