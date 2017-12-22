#include <stdexcept>
#include "patcher.hpp"
#include "internal.hpp"

namespace dk {
	object_patcher::object_patcher(POBJECT_HEADER header) : header_(header) {
		header_->KernelObject = 0;
		header_->KernelOnlyAccess = 0;
	}

	object_patcher::~object_patcher() {
		header_->KernelObject = 1;
		header_->KernelOnlyAccess = 1;
	}

	acl_patcher::acl_patcher(HANDLE handle) : handle_(handle) {
		if (GetSecurityInfo(handle_, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &previous_dacl_, NULL, 0) != ERROR_SUCCESS) {
			throw std::runtime_error("Unable to get security info");
		}

		PACL dacl = NULL;
		EXPLICIT_ACCESS	access_entry = {
			SECTION_ALL_ACCESS, GRANT_ACCESS, NO_INHERITANCE,
			{
				NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_NAME, TRUSTEE_IS_USER, "CURRENT_USER"
			}
		};

		if (SetEntriesInAcl(1, &access_entry, previous_dacl_, &dacl) != ERROR_SUCCESS) {
			throw std::runtime_error("Unable to set ACL entry");
		}

		if (SetSecurityInfo(handle_, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, dacl, NULL) != ERROR_SUCCESS) {
			throw std::runtime_error("Unable to set security info");
		}
	}

	acl_patcher::~acl_patcher() {
		if (SetSecurityInfo(handle_, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, previous_dacl_, NULL) != ERROR_SUCCESS) {
			return;
		}

		CloseHandle(handle_);
	}
}
