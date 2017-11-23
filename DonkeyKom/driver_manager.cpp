#include <stdexcept>
#include "driver_manager.hpp"

namespace dk {
	driver_manager::driver_manager() : driver_handle_(0) {

	}

	driver_manager::~driver_manager() {
		close_driver();
	}

	void driver_manager::open_driver() {
		if (driver_handle_ != NULL) {
			throw std::runtime_error("Driver already opened");
		}

		driver_handle_ = CreateFileA("\\\\.\\ASMMAP64",
			GENERIC_READ | GENERIC_WRITE | FILE_GENERIC_EXECUTE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, 0);
		if (driver_handle_ == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Unable to open handle to driver");
		}
	}

	void driver_manager::close_driver() {
		if (driver_handle_ != NULL) {
			CloseHandle(driver_handle_);
			driver_handle_ = NULL;
		}
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
