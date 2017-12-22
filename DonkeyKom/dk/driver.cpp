#pragma comment(lib, "Shlwapi.lib")
#include <Shlwapi.h>
#include <stdexcept>
#include "driver.hpp"
#include "internal.hpp"

namespace dk {
	driver_manager::driver_manager() {
		// Check if the driver is already running
		driver_handle_ = CreateFileA("\\\\.\\ASMMAP64",
			GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, 0);
		if (driver_handle_ == INVALID_HANDLE_VALUE) {
			add_process_privilege();

			create_reg_key();

			// Load service
			auto service_name = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\asmmap64");
			auto status = NtLoadDriver(&service_name);
			if (!NT_SUCCESS(status)) {
				SHDeleteKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\asmmap64");

				throw std::runtime_error("Unable to load driver");
			}

			installed_ = true;

			// Try opening a handle again
			driver_handle_ = CreateFileA("\\\\.\\ASMMAP64",
				GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, 0);
			if (driver_handle_ == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Unable to open handle to driver");
			}
		}
	}

	driver_manager::~driver_manager() {
		CloseHandle(driver_handle_);

		if (installed_) {
			auto service_name = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\asmmap64");
			NtUnloadDriver(&service_name);
			SHDeleteKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\asmmap64");
		}
	}

	void driver_manager::add_process_privilege() {
		TOKEN_PRIVILEGES privilege = { 1,{ { NULL, NULL, SE_PRIVILEGE_ENABLED } } };
		if (!LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &privilege.Privileges[0].Luid)) {
			throw std::runtime_error("Unable to lookup privilege");
		}

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_)) {
			throw std::runtime_error("Unable to open process token");
		}

		DWORD returned_length;
		if (!AdjustTokenPrivileges(token_, FALSE, &privilege, sizeof(privilege), &previous_privileges_, &returned_length)) {
			throw std::runtime_error("Unable to adjust token privileges");
		}
	}

	void driver_manager::create_reg_key() {
		auto key_path = "System\\CurrentControlSet\\Services\\asmmap64";

		HKEY key;
		DWORD disposition;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, key_path, NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &key, &disposition)) {
			throw std::runtime_error("Unable to create registry key");
		}

		char driver_path[_MAX_PATH];
		_fullpath(driver_path, "asmmap64.sys", _MAX_PATH);
		std::string full_driver_path{ driver_path };
		full_driver_path.insert(0, "\\??\\");

		if (RegSetValueEx(key, "ImagePath", NULL, REG_EXPAND_SZ,
			reinterpret_cast<const BYTE *>(full_driver_path.c_str()), full_driver_path.length())) {
			throw std::runtime_error("Unable to set ImagePath registry value");
		}

		DWORD service_type = 1;

		if (RegSetValueEx(key, "Type", NULL, REG_DWORD,
			reinterpret_cast<const BYTE *>(&service_type), sizeof(service_type))) {
			throw std::runtime_error("Unable to set Type registry value");
		}

		DWORD error_control = 1;

		if (RegSetValueEx(key, "ErrorControl", NULL, REG_DWORD,
			reinterpret_cast<const BYTE *>(&error_control), sizeof(error_control))) {
			throw std::runtime_error("Unable to set ErrorControl registry value");
		}

		DWORD start_type = 3;

		if (RegSetValueEx(key, "Start", NULL, REG_DWORD,
			reinterpret_cast<const BYTE *>(&start_type), sizeof(start_type))) {
			throw std::runtime_error("Unable to set Start registry value");
		}

		RegCloseKey(key);
	}

	uint8_t *driver_manager::map_memory(uint64_t addr, uint32_t length) {
		asmmap_ioctl ioctl = { addr, nullptr,{ length, length } };

		DWORD bytes_returned;
		if (!DeviceIoControl(driver_handle_, ASMMAP_IOCTL_MAPMEM, &ioctl, sizeof(ioctl), &ioctl, sizeof(ioctl), &bytes_returned, 0)) {
			throw std::runtime_error("Unable to map memory");
		}

		return ioctl.virtual_addr;
	}

	void driver_manager::unmap_memory(uint8_t *data, uint32_t length) {
		asmmap_ioctl ioctl = { 0, data,{ length, length } };

		DWORD bytes_returned;
		if (!DeviceIoControl(driver_handle_, ASMMAP_IOCTL_UNMAPMEM, &ioctl, sizeof(ioctl), &ioctl, sizeof(ioctl), &bytes_returned, 0)) {
			throw std::runtime_error("Unable to unmap memory");
		}
	}
}
