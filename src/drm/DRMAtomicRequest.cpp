#include "drm.h"
#include <cstring>

namespace drm {

DRMAtomicRequest::DRMAtomicRequest(const DRMCard& card) : card{card}, req{drmModeAtomicAlloc()} {
    if (!card.are_atomic_commits_enabled()) {
        throw DRMException{"refusing to create atomic request: atomic commits are not enabled"};
    }

    if (!req) {
        throw DRMException{"failed to allocate memory for atomic request", errno};
    }
}

DRMAtomicRequest::~DRMAtomicRequest() {
    drmModeAtomicFree(req);
}

void DRMAtomicRequest::add_property(const uint32_t obj_id, const uint32_t obj_type, const char* prop_name,
    const uint64_t val) const
{
    uint32_t prop_id {0};
    const auto props {drmModeObjectGetProperties(card.get_fd(), obj_id, obj_type)};
    if (!props) {
        throw DRMException{"failed to get properties"};
    }

    for (uint32_t i {0}; i < props->count_props; i++) {
        auto prop {drmModeGetProperty(card.get_fd(), props->props[i])};
        if (!prop) {
            throw DRMException{"failed to get property"};
        }
        if (strcmp(prop->name, prop_name) == 0) {
            prop_id = prop->prop_id;
            break;
        }
    }

    if (!prop_id) {
        throw DRMException{std::string("cannot add property: ") + prop_name + " does not exist"};
    }

    const auto res {drmModeAtomicAddProperty(req, obj_id, prop_id, val)};
    if (res == -EINVAL) {
        throw DRMException{"failed to add property to atomic request: atomic commits are not enabled"};
    } else if (res == -1) {
        throw DRMException{"failed to add property to atomic request", errno};
    }
}

void DRMAtomicRequest::commit(const uint32_t flags) const {
    const auto res {drmModeAtomicCommit(card.get_fd(), req, flags, nullptr)};
    if (res == -1) {
        throw DRMException{"failed to modeset atomically: null request"};
    } else if (res == -EINVAL) {
        throw DRMException{"failed to modeset atomically", errno};
    }
}

void DRMAtomicRequest::commit() const {
    commit(0);
}

}
