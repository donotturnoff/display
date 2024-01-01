#include "drm.h"
#include <iostream>

namespace drm {

// TODO: remove DRMCardFile
DRMCard::DRMCard(const std::string& path) : file{path} {}

int DRMCard::get_fd() const noexcept {
    return file.get_fd();
}

std::vector<DRMCRTC>& DRMCard::get_crtcs() noexcept {
    return crtcs;
}

std::vector<DRMEncoder>& DRMCard::get_encoders() noexcept {
    return encoders;
}

void DRMCard::set_capabilities() {
    // TODO: set capabilities in one place
    // TODO: handle the cases where setting capabilities fail
    // TODO: make this a config option?
    // TODO: structured logging
    if (drmSetClientCap(get_fd(), DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0) {
        std::cerr << "failed to enable universal planes, continuing" << std::endl;
    }

    if (drmSetClientCap(get_fd(), DRM_CLIENT_CAP_ATOMIC, 1) < 0) {
        // TODO: handle failure gracefully
        throw DRMException{"failed to enable atomic commits"};
    }
}

void DRMCard::load_resources() {
    connectors.clear();
    encoders.clear();
    crtcs.clear();
    planes.clear();

    DRMModeResUniquePtr res {drmModeGetResources(file.get_fd()), drmModeFreeResources};
    if (!res) {
        throw DRMException{"cannot fetch resources for card", errno};
    }

    for (auto i {0}; i < res->count_connectors; i++) {
        auto id {res->connectors[i]};
        DRMModeConnUniquePtr conn {drmModeGetConnector(file.get_fd(), id), drmModeFreeConnector};
        if (!conn) {
            throw DRMException{"cannot fetch connector", errno};
        }
        connectors.emplace_back(*this, conn.get());
    }

    for (auto i {0}; i < res->count_encoders; i++) {
        auto id {res->encoders[i]};
        DRMModeEncoderUniquePtr encoder {drmModeGetEncoder(file.get_fd(), id), drmModeFreeEncoder};
        if (!encoder) {
            throw DRMException{"cannot fetch encoder", errno};
        }
        encoders.emplace_back(encoder.get());
    }

    for (auto i {0}; i < res->count_crtcs; i++) {
        auto id {res->crtcs[i]};
        DRMModeCRTCUniquePtr crtc {drmModeGetCrtc(file.get_fd(), id), drmModeFreeCrtc};
        if (!crtc) {
            throw DRMException{"cannot fetch CRTC", errno};
        }
        crtcs.emplace_back(crtc.get());
    }

    DRMModePlaneResUniquePtr plane_res {drmModeGetPlaneResources(file.get_fd()), drmModeFreePlaneResources};
    if (!plane_res) {
        throw DRMException{"cannot fetch plane resources for card", errno};
    }

    for (auto i {0}; i < plane_res->count_planes; i++) {
        auto id {plane_res->planes[i]};
        DRMModePlaneUniquePtr plane {drmModeGetPlane(file.get_fd(), id), drmModeFreePlane};
        if (!plane) {
            throw DRMException{"cannot fetch plane", errno};
        }
        planes.emplace_back(plane.get());
    }
}

void DRMCard::configure_connectors() noexcept {
    for (auto& conn: connectors) {
        if (!conn.is_connected()) {
            continue;
        }

        try {
            // TODO: move this logic to select_crtc?
            auto crtc {conn.get_crtc()};
            if (!crtc) {
                crtc = conn.select_crtc();
            }

            conn.modeset();
        } catch (const DRMException& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

// TODO: remove? add to new DRMAtomicReq class?
void DRMCard::add_property(drmModeAtomicReq* req, const uint32_t object_id,
        const uint32_t object_type, const char* prop_name, const uint64_t value) {

    uint32_t prop_id {0};
    auto props {drmModeObjectGetProperties(get_fd(), object_id, object_type)};
    for (auto i {0}; i < props->count_props; i++) {
        auto prop {drmModeGetProperty(fd, props->props[i])};
        if (strcmp(prop->name, prop_name) == 0) {
            prop_id = prop->prop_id;
            break;
        }
    }

    if (!prop_id) {
        throw DRMException{"property " + prop_name + " does not exist"};
    }

    drmModeAtomicAddProperty(req, object_id, prop_id, value);
}

uint64_t DRMCard::fetch_capability(const uint64_t capability) const {
    uint64_t value;
    if (drmGetCap(file.get_fd(), capability, &value) < 0) {
        throw DRMException{"cannot check capability"};
    }
    return value;
}

bool DRMCard::supports_async_page_flip() const {
    return fetch_capability(DRM_CAP_ASYNC_PAGE_FLIP) == 1;
}

bool DRMCard::supports_dumb_buffers() const {
    return fetch_capability(DRM_CAP_DUMB_BUFFER) == 1;
}

bool DRMCard::supports_monotonic_timestamp() const {
    return fetch_capability(DRM_CAP_TIMESTAMP_MONOTONIC) == 1;
}

// TODO: use these
/* Hint to userspace of max cursor width */
uint64_t DRMCard::max_cursor_width() const {
    return fetch_capability(DRM_CAP_CURSOR_WIDTH);
}

/* Hint to userspace of max cursor height */
uint64_t DRMCard::max_cursor_height() const {
    return fetch_capability(DRM_CAP_CURSOR_HEIGHT);
}

}
