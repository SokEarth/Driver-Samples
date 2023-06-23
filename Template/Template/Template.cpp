// Copy and Paste this template code
#define UNICODE
#define _UNICODE
#include <ntddk.h>

void TemplateUnload(_In_ PDRIVER_OBJECT);
NTSTATUS TemplateCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP);

extern "C"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	// UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	// Initialise dispatch routines
	DriverObject->DriverUnload = TemplateUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = TemplateCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = TemplateCreateClose;

	// Create device object
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\TemplateDriver");
	
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject
		0
		&devName;
		FILE_DEVICE_UNKNOWN
		0
		FALSE
		&DeviceObject
	);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}
	
	// Create symbolic link
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\TemplateDriver");
	status = IoCreateSymbolicLink(&symLink, &devName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	KdPrint(("Template Driver Initialised Successfully"));

	return STATUS_SUCCESS;
}

void TemplateUnload(_In_ PDRIVER_OBJECT DriverObject) {
	// UNREFERENCED_PARAMETER(DriverObject);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\TemplateDriver");
	IoDeleteSymbolicLink(&symLink);

	IoDeleteDevice(DriverObject->DeviceObject);
	
	KdPrint(("Template Driver Unload Called"));
}

NTSTATUS TemplateCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
