#include "drm.h"
#include <fcntl.h>
#include <iostream>

namespace drm {

DRMCard::DRMCard(const std::string& path) : fd{open_device(path)} {}

DRMCard::~DRMCard() {
    // TODO: are these necessary?
    connectors.clear();
    encoders.clear();
    crtcs.clear();
    planes.clear();

	close(fd);
}

DRMCRTC& DRMCard::get_connected_crtc() {
    for (auto& [id, crtc]: crtcs) {
        if (crtc.is_connected()) {
            return crtc;
        }
    }

    throw DRMException{"no connected CRTC found"};
}

DRMCRTC& DRMCard::get_crtc_by_id(const uint32_t id) {
    return crtcs.at(id);
}

DRMEncoder& DRMCard::get_encoder_by_id(const uint32_t id) {
    return encoders.at(id);
}

// TODO: reduce duplication
DRMPlane& DRMCard::get_unused_overlay_plane(const DRMCRTC& crtc) {
    // Iterating over plane_ids vector allows us to go in order. TODO: is this necessary?
    for (const auto id: plane_ids) {
        auto& plane {planes.at(id)};
        if (plane.is_overlay_plane() && !plane.is_in_use() && plane.is_compatible_with(crtc)) {
            return plane;
        }
    }

    throw DRMException{"no unused overlay planes compatible with CRTC #" + std::to_string(crtc.get_id())};
}

DRMPlane& DRMCard::get_unused_primary_plane(const DRMCRTC& crtc) {
    for (const auto id: plane_ids) {
        auto& plane {planes.at(id)};
        if (plane.is_primary_plane() && !plane.is_in_use() && plane.is_compatible_with(crtc)) { // TODO: what about multiple primary planes? Which to return?
            return plane;
        }
    }

    throw DRMException{"no unused primary planes compatible with CRTC #" + std::to_string(crtc.get_id())};
}

DRMPlane& DRMCard::get_unused_cursor_plane(const DRMCRTC& crtc) {
    for (const auto id: plane_ids) {
        auto& plane {planes.at(id)};
        if (plane.is_cursor_plane() && !plane.is_in_use() && plane.is_compatible_with(crtc)) { // TODO: what about multiple cursor planes? Which to return?
            return plane;
        }
    }

    throw DRMException{"no unused cursor planes compatible with CRTC #" + std::to_string(crtc.get_id())};
}

void DRMCard::set_capabilities() {
    if (!supports_dumb_buffers()) {
        throw DRMException{"dumb buffers not supported"};
    }

    // TODO: set capabilities in one place
    // TODO: handle the cases where setting capabilities fail
    // TODO: make this a config option?
    // TODO: structured logging
    try {
        enable_universal_planes(); // TODO: for universal planes, ensure each CRTC gets its primary plane
    } catch (const DRMException& e) {
        std::cerr << e.what() << " (continuing)" << std::endl;
    }

    try {
        enable_atomic_commits();
    } catch (const DRMException& e) {
        std::cerr << e.what() << " (continuing)" << std::endl;
    }
}

void DRMCard::load_resources() {
    connectors.clear();
    encoders.clear();
    crtcs.clear();
    crtc_ids.clear();
    planes.clear();

    DRMModeResUniquePtr res {drmModeGetResources(fd), drmModeFreeResources};
    if (!res) {
        throw DRMException{"cannot fetch resources for card", errno};
    }

    for (auto i {0}; i < res->count_connectors; i++) {
        const auto id {res->connectors[i]};
        connectors.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id));
    }

    for (auto i {0}; i < res->count_encoders; i++) {
        const auto id {res->encoders[i]};
        encoders.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id));
    }

    DRMModePlaneResUniquePtr plane_res {drmModeGetPlaneResources(fd), drmModeFreePlaneResources};
    if (!plane_res) {
        throw DRMException{"cannot fetch plane resources for card", errno};
    }

    for (uint32_t i {0}; i < plane_res->count_planes; i++) {
        const auto id {plane_res->planes[i]};
        planes.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id));
        plane_ids.push_back(id);
    }

    // Do CRTCs after planes, because the CRTC constructor needs the planes to be loaded to find the primary plane
    for (auto i {0}; i < res->count_crtcs; i++) {
        const auto id {res->crtcs[i]};
        crtcs.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id, i));
        crtc_ids.push_back(id);
    }
}

void DRMCard::configure_connectors() noexcept {
    for (auto& [id, conn]: connectors) {
        if (!conn.is_connected()) {
            continue;
        }

        try {
            auto& crtc {conn.select_crtc()};
            crtc.modeset(conn);
        } catch (const DRMException& e) {
            std::cerr << "failed to configure connector #" << id << ": " << e.what() << std::endl;
        }
    }
}

uint64_t DRMCard::fetch_capability(const uint64_t capability) const {
    uint64_t value;
    if (drmGetCap(fd, capability, &value) < 0) {
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

void DRMCard::enable_universal_planes() {
    if (drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0) {
        throw DRMException{"failed to enable universal planes"};
    }
}

void DRMCard::enable_atomic_commits() {
    if (drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1) < 0) {
        throw DRMException{"failed to enable atomic commits"};
    }
    atomic_commits_enabled = true;
}

int DRMCard::open_device(const std::string& path) const {
	auto fd {open(path.c_str(), O_CLOEXEC | O_RDWR)};
	if (fd < 0) {
		throw DRMException{"cannot open card", errno};
	}
	return fd;
}

}
