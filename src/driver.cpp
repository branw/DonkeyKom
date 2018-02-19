#include <Windows.h>
#include <stdexcept>
#include <Shlwapi.h>
#include "dk/driver.hpp"
#include "internal.hpp"

namespace dk
{
	privilege_adjuster::privilege_adjuster(ULONG privilege) : privilege_(privilege)
	{
		BOOLEAN previous;
		if (!NT_SUCCESS(RtlAdjustPrivilege(privilege, TRUE, TRUE, &previous)))
		{
			throw std::runtime_error("Unable to acquire thread privilege");
		}
	}

	privilege_adjuster::~privilege_adjuster()
	{
		BOOLEAN previous;
		RtlAdjustPrivilege(privilege_, FALSE, TRUE, &previous);
	}

	driver::driver(const std::string &device_path)
	{
		driver_handle_ = CreateFile(device_path.c_str(), GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, nullptr,
		                            OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, nullptr);
		if (driver_handle_ == INVALID_HANDLE_VALUE)
		{
			throw std::runtime_error("Cannot open handle to the driver");
		}
	}

	driver::driver(const std::string &device_path, const std::experimental::filesystem::path &driver_path)
	{
		driver_handle_ = CreateFile(device_path.c_str(), GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, nullptr,
		                            OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, nullptr);
		if (driver_handle_ == INVALID_HANDLE_VALUE)
		{
			// Install the driver if we can't already find it
			{
				privilege_adjuster priv(SeLoadDriverPrivilege);
				load_driver(driver_path);

				installed_ = true;
			}

			// Try opening the handle again
			driver_handle_ = CreateFile(device_path.c_str(), GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, nullptr,
			                            OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, nullptr);
			if (driver_handle_ == INVALID_HANDLE_VALUE)
			{
				// Something went wrong, undo what we just tried doing
				{
					//TODO is this necessary?
					privilege_adjuster priv(SeLoadDriverPrivilege);
					unload_driver();
				}

				throw std::runtime_error("Could not open handle to driver after installation");
			}
		}
	}

	driver::~driver()
	{
		CloseHandle(driver_handle_);

		if (!installed_)
		{
			//TODO is this necessary?
			privilege_adjuster priv(SeLoadDriverPrivilege);
			unload_driver();
		}
	}

	void driver::load_driver(const std::experimental::filesystem::path &driver_path)
	{
		// Create registry key for driver
		driver_name_ = driver_path.stem().string();
		const auto key_path = R"(System\CurrentControlSet\Services\)" + driver_name_;

		HKEY key;
		DWORD disposition;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, key_path.c_str(), NULL, nullptr, NULL, KEY_ALL_ACCESS, nullptr, &key,
		                   &disposition))
		{
			throw std::runtime_error("Unable to create driver registry key");
		}

		// Set registry key values
		auto full_driver_path = "\\??\\" + driver_path.string();
		DWORD service_type = 1, error_control = 1, start_type = 3;

		if (RegSetValueEx(key, "ImagePath", NULL, REG_EXPAND_SZ, reinterpret_cast<const BYTE *>(full_driver_path.c_str()),
		                  full_driver_path.length()) ||
			RegSetValueEx(key, "Type", NULL, REG_DWORD, reinterpret_cast<const BYTE *>(&service_type), sizeof(service_type)) ||
			RegSetValueEx(key, "ErrorControl", NULL, REG_DWORD, reinterpret_cast<const BYTE *>(&error_control),
			              sizeof(error_control)) ||
			RegSetValueEx(key, "Start", NULL, REG_DWORD, reinterpret_cast<const BYTE *>(&start_type), sizeof(start_type)))
		{
			// Delete the key if setting the values failed
			RegCloseKey(key);
			SHDeleteKey(HKEY_LOCAL_MACHINE, key_path.c_str());
			throw std::runtime_error("Unable to set driver register values");
		}

		RegCloseKey(key);

		// Convert name to wide-character format
		auto service_name = R"(\Registry\Machine\)" + key_path;
		const auto length = MultiByteToWideChar(CP_ACP, NULL,
			service_name.c_str(), service_name.length() + 1, nullptr, NULL);
		const auto buffer = new wchar_t[length];
		MultiByteToWideChar(CP_ACP, NULL,
			service_name.c_str(), service_name.length() + 1, buffer, length);

		// Start the driver
		UNICODE_STRING unicode_service_name;
		RtlInitUnicodeString(&unicode_service_name, buffer);
		if (!NT_SUCCESS(NtLoadDriver(&unicode_service_name)))
		{
			SHDeleteKey(HKEY_LOCAL_MACHINE, key_path.c_str());
			throw std::runtime_error("Unable to load the driver");
		}
	}

	void driver::unload_driver() const
	{
		const auto key_path = R"(System\CurrentControlSet\Services\)" + driver_name_;
		SHDeleteKey(HKEY_LOCAL_MACHINE, key_path.c_str());
	}

	asmmap_driver::asmmap_driver() : driver(R"(\\.\ASMMAP64)")
	{
	}

	asmmap_driver::asmmap_driver(const std::experimental::filesystem::path &driver_path) : driver(R"(\\.\ASMMAP64)", driver_path)
	{
	}
}
