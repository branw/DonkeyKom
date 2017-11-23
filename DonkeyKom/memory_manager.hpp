#pragma once
#include <functional>

namespace dk {
	struct memory_range {
		uint64_t begin;
		uint64_t end;
		uint64_t length;
	};

	struct memory_manager {
		using CursorCallback = std::function<bool(uint64_t, uint8_t *&)>;
		using Callback = std::function<bool(uint64_t)>;

		memory_manager();
		~memory_manager();

		void populate_ranges();

		uint64_t scan_ranges(char *pool_tag, CursorCallback page_cb, CursorCallback block_cb);

	private:
		uint8_t prof_privilege_, debug_privilege_;

		memory_range ranges_[32];
		int num_ranges_;

	};
}
