#include <stdexcept>
#include "memory.hpp"
#include "internal.hpp"

namespace dk {
	memory_manager::memory_manager() : mapped_memory_(nullptr) {
		auto status = RtlAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE, FALSE, &prof_privilege_);
		status |= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &debug_privilege_);
		if (!NT_SUCCESS(status)) {
			throw std::runtime_error("Unable to adjust privileges");
		}

		populate_ranges();
	}

	memory_manager::~memory_manager() {
		auto status = RtlAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, prof_privilege_, FALSE, &prof_privilege_);
		status |= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, debug_privilege_, FALSE, &debug_privilege_);

		if (mapped_memory_ != nullptr) {
			//TODO unmap memory
		}

		if (heap_ != nullptr) {
			HeapFree(GetProcessHeap(), NULL, heap_);
		}
	}

	void memory_manager::populate_ranges() {
		SUPERFETCH_INFORMATION superfetch_info;
		PPF_MEMORY_RANGE_INFO range_infos;
		PF_MEMORY_RANGE_INFO range_info;

		superfetch_info = {
			SUPERFETCH_VERSION, SUPERFETCH_MAGIC,
			SuperfetchMemoryRangesQuery, &range_info, sizeof(range_info)
		};

		// We can expect for the system to allocate pages in several ranges
		auto result_length = 0UL;
		if (NtQuerySystemInformation(SystemSuperfetchInformation,
			&superfetch_info, sizeof(superfetch_info), &result_length) == STATUS_BUFFER_TOO_SMALL) {
			heap_ = HeapAlloc(GetProcessHeap(), 0, result_length);
			range_infos = static_cast<PPF_MEMORY_RANGE_INFO>(heap_);
			range_infos->Version = 1;

			superfetch_info = {
				SUPERFETCH_VERSION, SUPERFETCH_MAGIC,
				SuperfetchMemoryRangesQuery, range_infos, result_length
			};

			if (!NT_SUCCESS(NtQuerySystemInformation(SystemSuperfetchInformation,
				&superfetch_info, sizeof(superfetch_info), &result_length))) {
				throw std::runtime_error("Unable to superfetch ranges");
			}
		}
		else {
			range_infos = &range_info;
		}

		num_ranges_ = range_infos->RangeCount;
		for (auto i = 0; i < num_ranges_; i++) {
			auto node = reinterpret_cast<PPHYSICAL_MEMORY_RUN>(&range_infos->Ranges[i]);

			// Each page is 4KiB 
			const auto page_size = 4096;

			ranges_[i] = {
				node->BasePage * page_size,
				(node->BasePage + node->PageCount) * page_size
			};
		}

		ranges_[num_ranges_ - 1].end -= 0x1000;

		if (heap_ != nullptr) {
			HeapFree(GetProcessHeap(), NULL, heap_);
			heap_ = nullptr;
		}
	}

	uint64_t memory_manager::scan_ranges(char *pool_tag, size_t page_size, Callback page_cb,
		Callback block_cb) {
		if (num_ranges_ == 0) {
			throw std::runtime_error("No memory ranges to scan");
		}

		// Pack the pool tag into a DWORD
		const auto encoded_pool_tag = (
			pool_tag[0] | pool_tag[1] << 8 | pool_tag[2] << 16 | pool_tag[3] << 24
			);

		// Iterate over all memory ranges in chunks
		auto end_addr = ranges_[num_ranges_ - 1].end;
		for (auto addr = 0ULL; addr < end_addr; addr += page_size) {
			// Check that the address exists in an actual range
			if (!is_valid_addr(addr)) {
				continue;
			}

			uint8_t *cursor = nullptr;
			if (!page_cb(addr, cursor)) {
				break;
			}

			// Iterate over blocks of the pool
			auto start_cursor = cursor;
			auto actual_prev_block_size = 0;
			while (cursor - start_cursor < page_size) {
				auto pool_header = reinterpret_cast<PPOOL_HEADER>(cursor);
				auto block_size = pool_header->BlockSize << 4;
				auto prev_block_size = pool_header->PreviousSize << 4;

				if (prev_block_size != actual_prev_block_size ||
					block_size == 0 || block_size >= 0x0FFF) {
					break;
				}

				actual_prev_block_size = block_size;

				// The MSB is reserved for the allocator and should be discarded
				if ((pool_header->PoolTag & 0x7FFFFFFF) == encoded_pool_tag) {
					if (!block_cb(addr + (cursor - start_cursor), cursor)) {
						return reinterpret_cast<uint64_t>(cursor);
					}
				}

				cursor += block_size;
			}
		}

		return 0;
	}

	void memory_manager::map_all_memory(HANDLE handle, uint8_t *&memory) {
		SIZE_T length = ranges_[num_ranges_ - 1].end;
		LARGE_INTEGER section_offset = { 0 };
		memory = 0;

		auto status = NtMapViewOfSection(handle, GetCurrentProcess(), reinterpret_cast<PVOID *>(&memory),
			NULL, length, &section_offset, &length, ViewShare, NULL, PAGE_READWRITE);
		if (!NT_SUCCESS(status)) {
			throw std::runtime_error("Unable to map memory view");
		}

		mapped_memory_ = memory;
	}


	bool memory_manager::is_valid_addr(uint64_t addr) {
		// Naively find if the address falls within a range
		for (auto i = 0; i < num_ranges_; i++) {
			if (ranges_[i].begin <= addr && addr <= ranges_[i].end) {
				return true;
			}
		}

		return false;
	}
}
