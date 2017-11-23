#pragma once
#include <Windows.h>

#include "driver_manager.hpp"
#include "memory_manager.hpp"

namespace dk {
	void initialize();

	HANDLE open_physical_mem(ACCESS_MASK access = SECTION_ALL_ACCESS);

	PACL patch_phys_mem_security_descriptor();

	BOOLEAN unpatch_phys_mem_security_descriptor(PACL previous_dacl);
}
