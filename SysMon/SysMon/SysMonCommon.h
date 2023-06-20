#pragma once
#include <winnt.h>
#include <windef.h>

enum class ItemType : short {
	None,
	ProcessCreate,
	ProcessExit
};

struct ItemHeader {
	ItemType Type;
	USHORT Size;
	LARGE_INTEGER Time;
};