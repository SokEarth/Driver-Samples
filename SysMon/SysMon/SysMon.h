#pragma once

template <typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data; 
};