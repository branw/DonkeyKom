#include <stdexcept>
#include "memory_manager.hpp"
#include "memory_manager_internal.hpp"

namespace dk {
	memory_manager::memory_manager() {
		auto status = RtlAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE, FALSE, &prof_privilege_);
		status |= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &debug_privilege_);
		if (!NT_SUCCESS(status)) {
			throw std::runtime_error("Unable to adjust privileges");
		}
	}

	memory_manager::~memory_manager() {
		auto status = RtlAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, prof_privilege_, FALSE, &prof_privilege_);
		status |= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, debug_privilege_, FALSE, &debug_privilege_);
		if (!NT_SUCCESS(status)) {
			throw std::runtime_error("Unable to adjust privileges");
		}
	}

	void memory_manager::populate_ranges() {
		SUPERFETCH_INFORMATION superfetch_info;
		PPF_MEMORY_RANGE_INFO range_infos;
		PF_MEMORY_RANGE_INFO range_info;

		superfetch_info = { SUPERFETCH_VERSION, SUPERFETCH_MAGIC, SuperfetchMemoryRangesQuery, &range_info, sizeof(range_info) };

		auto result_length = 0UL;
		if (NtQuerySystemInformation(SystemSuperfetchInformation, &superfetch_info, sizeof(superfetch_info), &result_length) == STATUS_BUFFER_TOO_SMALL) {
			range_infos = static_cast<PPF_MEMORY_RANGE_INFO>(HeapAlloc(GetProcessHeap(), 0, result_length));
			range_infos->Version = 1;

			superfetch_info = { SUPERFETCH_VERSION, SUPERFETCH_MAGIC, SuperfetchMemoryRangesQuery, range_infos, result_length };
			if (!NT_SUCCESS(NtQuerySystemInformation(SystemSuperfetchInformation, &superfetch_info, sizeof(superfetch_info), &result_length))) {
				throw std::runtime_error("Unable to superfetch ranges");
			}
		}
		else {
			range_infos = &range_info;
		}

		num_ranges_ = range_infos->RangeCount;
		for (auto i = 0; i < num_ranges_; i++) {
			auto node = reinterpret_cast<PPHYSICAL_MEMORY_RUN>(&range_infos->Ranges[i]);

			ranges_[i] = {
				node->BasePage << 12,
				(node->BasePage + node->PageCount) << 12,
				((node->PageCount << 12) >> 10) * 1024
			};
		}
	}

	uint64_t memory_manager::scan_ranges(char *pool_tag, CursorCallback page_cb, CursorCallback block_cb) {
		if (num_ranges_ == 0) {
			throw std::runtime_error("No memory ranges to scan");
		}

		const uint64_t encoded_pool_tag = (pool_tag[0] | pool_tag[1] << 8 | pool_tag[2] << 16 | pool_tag[3] << 24);

		auto end_addr = ranges_[num_ranges_ - 1].end;
		for (auto addr = 0ULL; addr < end_addr; addr += 0x1000) {
			// Check that the address exists in an actual range
			//TODO improve
			auto does_addr_exist = false;
			for (auto i = 0; i < num_ranges_ && !does_addr_exist; i++) {
				if (ranges_[i].begin <= addr && addr <= ranges_[i].end) {
					does_addr_exist = true;
				}
			}
			if (!does_addr_exist) {
				continue;
			}

			uint8_t *cursor = nullptr;
			if (!page_cb(addr, cursor)) {
				break;
			}

			// Iterate over blocks of the pool
			auto start_cursor = cursor;
			auto actual_prev_block_size = 0;
			while (cursor - start_cursor < 0x1000) {
				auto pool_header = reinterpret_cast<PPOOL_HEADER>(cursor);
				auto block_size = pool_header->BlockSize << 4;
				auto prev_block_size = pool_header->PreviousSize << 4;

				if (prev_block_size != actual_prev_block_size ||
					block_size == 0 || block_size >= 0x0FFF) {
					break;
				}

				actual_prev_block_size = block_size;

				// Check if the object looks like PhysicalMemory
				if (pool_header->PoolTag == encoded_pool_tag) {
					if (!block_cb(addr + (cursor - start_cursor), cursor)) {
						return reinterpret_cast<uint64_t>(cursor);
					}
				}

				cursor += block_size;
			}
		}

		return 0;
	}
}
