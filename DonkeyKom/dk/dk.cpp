#pragma comment(lib, "ntdll.lib")
#include <wchar.h>
#include <string>
#include <vector>
#include "dk.hpp"
#include "internal.hpp"

namespace dk {
	HANDLE open_phys_mem(ACCESS_MASK access) {
		auto device_name = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");

		OBJECT_ATTRIBUTES attribs;
		InitializeObjectAttributes(&attribs, &device_name, NULL, NULL, NULL);

		HANDLE handle;
		auto status = NtOpenSection(&handle, access, &attribs);
		if (!NT_SUCCESS(status)) {
			throw std::runtime_error("Unable to open PhysicalMemory");
		}

		return handle;
	}

	donkey_kom::donkey_kom() {
		driver_manager driver;

		// Map the memory in small chunks
		auto page_size = 0x1000;
		auto last_mapped = driver.map_memory(0, page_size);
		auto page_cb = [&](uint64_t addr, uint8_t *&cursor) -> bool {
			driver.unmap_memory(last_mapped, page_size);
			last_mapped = cursor = driver.map_memory(addr, page_size);
			return true;
		};

		// Heuristically identify the PhysicalMemory object

		// It only has a NameInfo optional header
		auto offset = 0x10 /* _POOL_HEADER */ + 0x20 /* _OBJECT_HEADER_NAME_INFO */;

		auto block_cb = [=](uint64_t addr, uint8_t *&cursor) -> bool {
			auto obj_header = reinterpret_cast<POBJECT_HEADER>(cursor + offset);

			/*if (addr == 3297280) {
				throw std::runtime_error("hit");
			}*/

			// The SYSTEM process should have 2 handles to PhysicalMemory;
			// Only the KernelObject, KernelOnlyAccess, and PermanentObject flags are set
			if (obj_header->HandleCount >= 0 && obj_header->HandleCount <= 3 && obj_header->Flags == 0x16) {
				return false;
			}

			return true;
		};

		// \Device\PhsicalMemory isn't actually a Device but rather a Section
		auto phys_mem_obj = memory_.scan_ranges("Sect", page_size, page_cb, block_cb);
		if (!phys_mem_obj) {
			throw std::runtime_error("Unable to find PhysicalMemory");
		}

		{
			// Patch the object's header so it can be accessed from usermode
			object_patcher usermode_patch(reinterpret_cast<POBJECT_HEADER>(phys_mem_obj + offset));
			// We can now open a handle to it, but need to patch its ACL 
			acl_patcher security_patch(open_phys_mem(WRITE_DAC | READ_CONTROL));

			// Map the entirety of physical memory
			auto phys_mem = open_phys_mem(SECTION_ALL_ACCESS);
			memory_.map_all_memory(phys_mem, physical_memory_);
			CloseHandle(phys_mem);

			//kernel_cr3 = get_kernel_cr3();
		}

		// The chunk containing the object is still mapped at this point
		driver.unmap_memory(last_mapped, page_size);
	}

	donkey_kom::~donkey_kom() {
		//TODO unmap memory
	}

	bool donkey_kom::read(uint64_t addr, uint8_t *buffer, size_t length) {
		if (!memory_.is_valid_addr(addr)) {
			return false;
		}

		memcpy(buffer, reinterpret_cast<void *>(physical_memory_[addr]), length);
		return true;
	}

	bool donkey_kom::write(uint64_t addr, uint8_t *buffer, size_t length) {
		if (!memory_.is_valid_addr(addr)) {
			return false;
		}

		memcpy(reinterpret_cast<void *>(physical_memory_[addr]), buffer, length);
		return true;
	}

	bool donkey_kom::read_virtual(uint64_t cr3, uint64_t addr, uint8_t *buffer, size_t length) {
		return read(translate_virtual_addr(cr3, addr), buffer, length);
	}

	bool donkey_kom::write_virtual(uint64_t cr3, uint64_t addr, uint8_t *buffer, size_t length) {
		return write(translate_virtual_addr(cr3, addr), buffer, length);
	}

	uint64_t donkey_kom::get_eprocess(int pid) {
		uint64_t process = 0;

		uint64_t next_pid = 0;
		while (next_pid != 4) {
			uint64_t next_process = process + 0;
			//TODO
		}

		return process;
	}

	uint64_t donkey_kom::get_cr3(int pid) {
		uint64_t cr3 = 0;

		if (read_virtual(kernel_cr3_, get_eprocess(pid) + 0, reinterpret_cast<uint8_t *>(&cr3), sizeof(cr3))) {
			return cr3;
		}

		return 0;
	}

	uint64_t donkey_kom::get_kernel_cr3() {

		auto page_cb = [this](uint64_t addr, uint8_t *&cursor) -> bool {
			cursor = reinterpret_cast<uint8_t *>(physical_memory_[addr]);
			return true;
		};

		auto block_cb = [this](uint64_t addr, uint8_t *&cursor) -> bool {
			uint64_t process = 0;
			uint8_t buffer[0xFFFF];
			if (!read(addr, buffer, sizeof(buffer))) {
				return false;
			}

			for (uint8_t *iter = buffer; iter < buffer + sizeof(buffer); iter++) {
				if (!strcmp(reinterpret_cast<char *>(iter), "System")) {
					process = addr + (iter - buffer) - 0/*NameOffset*/;
					break;
				}
			}

			if (!process) {
				return true;
			}

			int pid = 0;
			if (!read(process + 0/*PidOffset*/, reinterpret_cast<uint8_t *>(&pid), sizeof(pid))) {
				return true;
			}

			if (pid != 4) {
				return true;
			}

			uint64_t cr3;
			if (!read(process + 0/*DirBaseOffset*/, reinterpret_cast<uint8_t *>(&cr3), sizeof(cr3))) {
				return true;
			}

			return true;
		};

		return memory_.scan_ranges("Proc", 0x1000, page_cb, block_cb);
	}

	uint64_t donkey_kom::translate_virtual_addr(uint64_t cr3, uint64_t virtual_addr) {
		int plm4 = (virtual_addr >> 39) & 0x1FF;
		int dir_ptr = (virtual_addr >> 30) & 0x1FF;
		int dir = (virtual_addr >> 21) & 0x1FF;
		int table = (virtual_addr >> 12) & 0x1FF;
		int addr = virtual_addr & 0x1FF;

		return 0;
	}
}
