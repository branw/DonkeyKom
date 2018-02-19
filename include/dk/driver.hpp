#pragma once
#include <Windows.h>
#include <cstdint>
#include <filesystem>

namespace dk
{
	// Adjusts the privilege token of the current thread
	struct privilege_adjuster
	{
		privilege_adjuster(ULONG privilege);

		~privilege_adjuster();

		privilege_adjuster(const privilege_adjuster &) = delete;
		privilege_adjuster(privilege_adjuster &&) = delete;
		privilege_adjuster &operator=(const privilege_adjuster &) = delete;
		privilege_adjuster &operator=(privilege_adjuster &&) = delete;

	private:
		ULONG privilege_;
	};

	// A connection to a driver
	struct driver
	{
		driver(const std::string &device_path);
		driver(const std::string &device_path, const std::experimental::filesystem::path &driver_path);

		~driver();

		driver(const driver &) = delete;
		driver(driver &&) = delete;
		driver &operator=(const driver &) = delete;
		driver &operator=(driver &&) = delete;

	private:
		HANDLE driver_handle_ = nullptr;

		bool installed_ = false;
		std::string driver_name_ = "";

		void load_driver(const std::experimental::filesystem::path &driver_path);
		void unload_driver() const;
	};

	// Something that can map and unmap memory
	struct memory_mapper
	{
		virtual ~memory_mapper() = default;

		virtual uint8_t *map_memory(uint64_t addr, uint32_t length) = 0;
		virtual void unmap_memory(uint8_t *data, uint32_t length) = 0;
	};

	// An ASMMAP driver instance
	struct asmmap_driver : driver, memory_mapper
	{
		asmmap_driver();
		asmmap_driver(const std::experimental::filesystem::path &driver_path);

	private:
		struct ioctl_info
		{
			uint64_t addr = 0;
			uint8_t *virtual_addr = nullptr;
			uint32_t length[2] = {0};
		};

		static const uint32_t ioctl_map_memory = 0x9C402580;
		static const uint32_t ioctl_unmap_memory = 0x9C402584;
	};
}
