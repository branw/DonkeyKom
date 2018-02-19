#include "dk/dk.hpp"

namespace dk
{
	phys_mem_mapper::phys_mem_mapper(const memory_mapper &mapper) : mapper_(mapper)
	{
	}

	phys_mem_mapper::~phys_mem_mapper()
	{
	}
}
