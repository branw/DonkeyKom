#pragma once
#include <Windows.h>
#include <functional>
#include "driver.hpp"
#include "memory.hpp"
#include "patcher.hpp"

namespace dk {
	HANDLE open_phys_mem(ACCESS_MASK access);

	struct donkey_kom {
		donkey_kom();
		~donkey_kom();

		bool read(uint64_t addr, uint8_t *buffer, size_t length);
		bool write(uint64_t addr, uint8_t *buffer, size_t length);

		bool read_virtual(uint64_t cr3, uint64_t addr, uint8_t *buffer, size_t length);
		bool write_virtual(uint64_t cr3, uint64_t addr, uint8_t *buffer, size_t length);

	private:
		uint64_t get_eprocess(int pid);

		uint64_t get_cr3(int pid);
		uint64_t get_kernel_cr3();

		uint64_t translate_virtual_addr(uint64_t cr3, uint64_t virtual_addr);

		memory_manager memory_;
		uint64_t kernel_cr3_ = 0;
		uint8_t *physical_memory_ = nullptr;
	};
}
