#pragma once
#include "driver.hpp"

namespace dk
{
	struct phys_mem_mapper
	{
		phys_mem_mapper(const memory_mapper &mapper);

		~phys_mem_mapper();

		phys_mem_mapper(const phys_mem_mapper &) = delete;
		phys_mem_mapper(phys_mem_mapper &&) = delete;
		phys_mem_mapper &operator=(const phys_mem_mapper &) = delete;
		phys_mem_mapper &operator=(phys_mem_mapper &&) = delete;

	private:
		const memory_mapper &mapper_;
	};
}
