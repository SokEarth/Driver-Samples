#pragma once
#include "SysMonCommon.h"

template <typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data; 
};

class FastMutex {
public:
	void Init();
	void Lock();
	void Unlock();

private:
	FAST_MUTEX _mutex;
};

void FastMutex::Init() {
	ExInitializeFastMutex(&_mutex);
}

void FastMutex::Lock() {
	ExAcquireFastMutex(&_mutex);
}

void FastMutex::Unlock() {
	ExReleaseFastMutex(&_mutex);
}

template<typename TLock>
struct AutoLock {
	AutoLock(TLock& lock) : _lock(lock) { 
		lock.Lock();
	}
	
	~AutoLock() { 
		_lock.Unlock();
	}

private:
	TLock& _lock;
};