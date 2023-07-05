#include "pch.h"
#include "FastMutex.h"

void FastMutex::Init() {
	ExInitializeFastmutex(&_mutex);
}

void FastMutex::Lock() {
	ExAcquireFastmutex(&_mutex);
}

void FastMutex::Unlock() {
	ExReleaseFastmutex(&_mutex);
}