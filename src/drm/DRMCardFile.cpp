#include "drm.h"
#include <fcntl.h>

namespace drm {

DRMCardFile::DRMCardFile(const std::string& path) : fd{open_device(path)} {}

DRMCardFile::~DRMCardFile() {
	close(fd);
}

int DRMCardFile::get_fd() const noexcept {
	return fd;
}

int DRMCardFile::open_device(const std::string& path) const {
	auto fd {open(path.c_str(), O_CLOEXEC | O_RDWR)};
	if (fd < 0) {
		throw DRMException{"cannot open card", errno};
	}
	return fd;
}

}
