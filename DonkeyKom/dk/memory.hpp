#pragma once
#include <Windows.h>
#include <functional>

namespace dk {
	struct memory_manager {
		using Callback = std::function<bool(uint64_t, uint8_t *&)>;

		memory_manager();
		~memory_manager();

		uint64_t scan_ranges(char *pool_tag, size_t page_size, Callback page_cb, Callback block_cb);

		void map_all_memory(HANDLE handle, uint8_t *&memory);

		bool is_valid_addr(uint64_t addr);

	private:
		uint8_t prof_privilege_, debug_privilege_;

		struct memory_range {
			uint64_t begin;
			uint64_t end;
		} ranges_[32];
		int num_ranges_;

		uint8_t *mapped_memory_;

		LPVOID heap_;

		void populate_ranges();
	};
}
