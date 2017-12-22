#pragma once
#include <Windows.h>
#include <cstdint>

namespace dk {
	HANDLE open_phys_mem(ACCESS_MASK access);

	struct driver_manager {
		driver_manager();
		~driver_manager();

		uint8_t *map_memory(uint64_t addr, uint32_t length);
		void unmap_memory(uint8_t *data, uint32_t length);

	private:
		void add_process_privilege();
		void create_reg_key();

		bool installed_ = false;
		HANDLE token_ = nullptr;
		TOKEN_PRIVILEGES previous_privileges_{ 0 };
		HANDLE driver_handle_ = nullptr;

		struct asmmap_ioctl {
			uint64_t addr = 0;
			uint8_t *virtual_addr = nullptr;
			uint32_t length[2] = { 0 };
		};

		const uint32_t ASMMAP_IOCTL_MAPMEM = 0x9C402580;
		const uint32_t ASMMAP_IOCTL_UNMAPMEM = 0x9C402584;
	};
}
