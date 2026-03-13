#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <vector>
#include <string>

// replcae this with your driver <3
inline uintptr_t base_addr;
inline uintptr_t cr3;

#define code_rw CTL_CODE(FILE_DEVICE_UNKNOWN,   0x42,  METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_ba CTL_CODE(FILE_DEVICE_UNKNOWN,   0x627, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_cr3 CTL_CODE(FILE_DEVICE_UNKNOWN,  0x67, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
//#define code_mouse CTL_CODE(FILE_DEVICE_UNKNOWN,0x69, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
typedef struct _rw {


	INT32 process_id;
	ULONGLONG address;
	ULONGLONG buffer;
	ULONGLONG size;
	BOOLEAN write;
} rw, * prw;

typedef struct _ba {

	INT32 process_id;
	ULONGLONG* address;
} ba, * pba;

typedef struct _ga {

	ULONGLONG* address;
} ga, * pga;

typedef struct _MEMORY_OPERATION_DATA {
	uint32_t        pid;
	ULONGLONG* cr3;
} MEMORY_OPERATION_DATA, * PMEMORY_OPERATION_DATA;

typedef struct _movemouse
{
	long x;
	long y;
	unsigned short button_flags;
} movemouse, * MouseMovementStruct;


namespace driver {
	inline HANDLE driver_handle;
	inline INT32 process_id;

	inline bool find_driver() {

		driver_handle = CreateFileW(L"\\\\.\\{A762D1D7-6A15-4B53-A2C3-DB1B7E3853EB}",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (!driver_handle || (driver_handle == INVALID_HANDLE_VALUE))
			return false;

		return true;
	}

	inline void read_physical(PVOID address, PVOID buffer, DWORD size) {
		_rw arguments = { 0 };


		arguments.address = (ULONGLONG)address;
		arguments.buffer = (ULONGLONG)buffer;
		arguments.size = size;
		arguments.process_id = process_id;
		arguments.write = FALSE;

		DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
	}
	// inline void mouse_move(int x, int y) {
	// 	_movemouse arguments = { 0 };
	// 	arguments.x = x;
	// 	arguments.y = y;

	// 	arguments.button_flags = 0;
	// 	DeviceIoControl(driver_handle, code_MOUSE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
	// }


	inline void write_physical(PVOID address, PVOID buffer, DWORD size) {
		_rw arguments = { 0 };


		arguments.address = (ULONGLONG)address;
		arguments.buffer = (ULONGLONG)buffer;
		arguments.size = size;
		arguments.process_id = process_id;
		arguments.write = TRUE;

		DeviceIoControl(driver_handle, code_rw, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);
	}



	inline uintptr_t fetch_cr3() {
		uintptr_t cr3 = NULL;
		_MEMORY_OPERATION_DATA arguments = { 0 };

		arguments.pid = process_id;
		arguments.cr3 = (ULONGLONG*)&cr3;

		DeviceIoControl(driver_handle, code_cr3, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

		return cr3;
	}

	inline uintptr_t find_image() {
		uintptr_t image_address = { NULL };
		_ba arguments = { NULL };

		arguments.process_id = process_id;
		arguments.address = (ULONGLONG*)&image_address;

		DeviceIoControl(driver_handle, code_ba, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

		return image_address;
	}


	inline HWND get_window(DWORD processId)
	{
		HWND hwnd = GetTopWindow(NULL);
		DWORD wndProcId = 0;
		while (hwnd)
		{
			GetWindowThreadProcessId(hwnd, &wndProcId);
			if (wndProcId == processId)
			{
				return hwnd;
			}
			hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
		}
		return NULL;
	}
	inline INT32 find_process(LPCTSTR process_name) {
		PROCESSENTRY32 pt;
		HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pt.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hsnap, &pt)) {
			do {
				if (!lstrcmpi(pt.szExeFile, process_name)) {
					CloseHandle(hsnap);
					process_id = pt.th32ProcessID;
					return pt.th32ProcessID;
				}
			} while (Process32Next(hsnap, &pt));
		}
		CloseHandle(hsnap);

		return { NULL };
	}
}

template <typename T>
inline T read(uint64_t address) {
	T buffer{ };
	driver::read_physical((PVOID)address, &buffer, sizeof(T));
	return buffer;
}

template <typename T>
inline T write(uint64_t address, T buffer) {

	driver::write_physical((PVOID)address, &buffer, sizeof(T));
	return buffer;
}
