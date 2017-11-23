#pragma once
#include <Windows.h>
#include <cstdint>

namespace dk {
	struct driver_manager {
		driver_manager();
		~driver_manager();

		void open_driver();

		void close_driver();

		uint8_t *map_memory(uint64_t addr, uint32_t length);

		void unmap_memory(uint8_t *data, uint32_t length);

	private:
		HANDLE driver_handle_;

		struct asmmap_ioctl {
			uint64_t addr = 0;
			uint8_t *virtual_addr = nullptr;
			uint32_t length[2] = { 0 };
		};

		const uint32_t ASMMAP_IOCTL_MAPMEM = 0x9C402580;
		const uint32_t ASMMAP_IOCTL_UNMAPMEM = 0x9C402584;
	};
}
