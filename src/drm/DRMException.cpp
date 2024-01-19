#include "drm.h"
#include <cstring>

namespace drm {

DRMException::DRMException(const std::string msg) noexcept : std::runtime_error(msg) {}

DRMException::DRMException(const int errnum) noexcept : std::runtime_error(std::strerror(errnum)) {}

DRMException::DRMException(const std::string msg, const int errnum) noexcept :
        std::runtime_error(msg + ": " + std::strerror(errnum)) {}

DRMException::DRMException(const std::string msg, const std::exception& e) noexcept :
        std::runtime_error(msg + ": " + e.what()) {}

DRMException::DRMException(const std::exception& e, const std::string extra) noexcept :
        std::runtime_error(std::string(e.what()) + " (also: " + extra + ")") {}

}
