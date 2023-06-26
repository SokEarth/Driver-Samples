int main() {
	auto hFile = ::CreateFile(L"\\\\.\\SysMon", GENERIC_READ, 0,
	nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open file");
		buffer[1 << 16]; // 64KB buffer
	while (true) {
		DWORD bytes;
		if (!::ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr))
		return Error("Failed to read");
		if (bytes != 0)
		DisplayInfo(buffer, bytes);
		::Sleep(200);
	}
}

void DisplayInfo(BYTE* buffer, DWORD size) {
	auto count = size;
	while (count > 0) {
	auto header = (ItemHeader*)buffer;
	switch (header->Type) {
		case ItemType::ProcessExit: {
			DisplayTime(header->Time);
			auto info = (ProcessExitInfo*)buffer;
			printf("Process %d Exited\n", info->ProcessId);
			break;
		}
		case ItemType::ProcessCreate: {
			DisplayTime(header->Time);
			auto info = (ProcessCreateInfo*)buffer;
			std::wstring commandline((WCHAR*)(buffer + info->CommandLineOffset), info->CommandLineLength);
			printf("Process %d Created. Command line: %ws\n", info->ProcessId, commandline.c_str());
			break;
		}
		default:
		break;
	}
	buffer += header->Size;
	count -= header->Size;
}

void DisplayTime(const LARGE_INTEGER& time) {
	SYSTEMTIME st;
	::FileTimeToSystemTime((FILETIME*)&time, &st);
	printf("%02d:%02d:%02d.%03d: ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}