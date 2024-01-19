#include "drm.h"
#include <iostream>

namespace drm {

DRMPropertyBlob::DRMPropertyBlob(const DRMCard& card, const drmModeModeInfo* mode) : card{card} {
    const auto res {drmModeCreatePropertyBlob(card.get_fd(), mode, sizeof(*mode), &id)};
    if (res == -1) {
        throw DRMException{"failed to create property blob: invalid data, size or id"};
    } else if (res == -ENOMEM) {
        throw DRMException{"failed to create property blob", errno};
    }
}

DRMPropertyBlob::~DRMPropertyBlob() {
    if (drmModeDestroyPropertyBlob(card.get_fd(), id) < 0) {
        std::cerr << "failed to destroy property blob" << std::endl;
    }
}

}
