#define DRIVER_PREFIX "Sysmon: "
#include <ntddk.h>
#include <wdm.h>
#include "SysMon.h"

struct Globals {
	LIST_ENTRY ItemsHead;
	int ItemCount;
	FastMutex Mutex;
};

struct ProcessExitInfo : ItemHeader {
	ULONG ProcessId;
};

struct ProcessCreateInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ParentProcessId;
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
};

Globals g_Globals;

void SysMonUnload(PDRIVER_OBJECT);
NTSTATUS SysMonRead(PDEVICE_OBJECT, PIRP);
NTSTATUS SysMonCreateClose(PDEVICE_OBJECT, PIRP);
void OnProcessNotify(PEPROCESS, HANDLE, PPS_CREATE_NOTIFY_INFO);
void PushItem(LIST_ENTRY*);

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	auto status = STATUS_SUCCESS;

	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\sysmon");
	bool symLinkCreated = false;
	
	do {
		status = IoCreateDevice(
			DriverObject,
			0,
			&devName,
			FILE_DEVICE_UNKNOWN,
			0,
			TRUE,
			&DeviceObject
		);

		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}

		DriverObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;

		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register callback (0x%08X)\n", status));
			break;
		}

	} while (false);

	if (!NT_SUCCESS(status)) {
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	DriverObject->DriverUnload = SysMonUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = SysMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;

	return status;
}

void SysMonUnload(PDRIVER_OBJECT DriverObject) {
	// unregister process notifications
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	// free remaining items
	while (!IsListEmpty(&g_Globals.ItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
}

NTSTATUS SysMonRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	auto count = 0;
	NT_ASSERT(Irp->MdlAddress); 
	// we're using Direct I/O
	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
	if (!buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}	else {
	AutoLock lock(g_Globals.Mutex); // C++ 17
	while (true) {
		if (IsListEmpty(&g_Globals.ItemsHead)) // can also check 
			g_Globals.ItemCount;
		break;
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
		auto size = info->Data.Size;
		if (len < size) {
			// user's buffer is full, insert item back
			InsertHeadList(&g_Globals.ItemsHead, entry);
			break;
		}
		g_Globals.ItemCount--;
		::memcpy(buffer, &info->Data, size);
		len -= size;
		buffer += size;
		count += size;
		// free data after
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = count;
		IoCompleteRequest(Irp, 0);
		return status;
}

NTSTATUS SysMonCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

}

void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	if (CreateInfo) {
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		if (CreateInfo->CommandLine) {
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}
		
		auto info = (FullItem<ProcessCreateInfo>*) ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTime(&item.Time);
		item.Type == ItemType::ProcessCreate;
		item.Size = sizeof(ProcessCreateInfo);
		item.ProcessId = HandleToULong(ProcessId);
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);

		if (commandLineSize > 0) {
			::memccpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);

		}
		else {
			item.CommandLineLength = 0;
		}

		PushItem(&info->Entry);
	} else {
		auto info = (FullItem<ProcessExitInfo>*) ExAllocatePoolWithTag(PagedPool, sizeof(FullItem <ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTime(&item.Time);
		item.Type == ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);

		PushItem(&info->Entry);
	}


}

void PushItem(LIST_ENTRY* Entry) {
	AutoLock <FastMutex> lock(g_Globals.Mutex);
	if (g_Globals.ItemCount > 1024) {
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ItemsHead>, Entry);
		ExFreePool(item);
	}
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}