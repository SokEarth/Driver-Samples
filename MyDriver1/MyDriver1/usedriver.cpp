#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include  "./PriorityBoosterCommon.h"

int Error(LPCTSTR);

int main(int argc, const char * argv[]) {
	ThreadData data;
	DWORD returned;
	if (argc < 3) {
		_ftprintf(stderr, _T("Usage: Booster <threadId> <priority>"));
		return 0;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
		return Error(_T("Failed to open device"));

	data.ThreadId = atoi(argv[1]);
	data.Priority = atoi(argv[2]);
	BOOL success = DeviceIoControl(hDevice,
		IOCTL_PRIORITY_BOOSTER_SET_PRIORITY,
		&data, sizeof(data),
		nullptr, 0,
		&returned, nullptr);
	if (success)
		_ftprintf(stdout, _T("Priority change succeeded!\n"));
	else
		Error(_T("Failed to open device"));
	CloseHandle(hDevice);
}

int Error(LPCTSTR message) {
	_ftprintf(stderr, _T("%s (error=%d)\n"), message, GetLastError());
	return 1;
}