#include "drm.h"
#include <iostream>

namespace drm {

DRMPlane::DRMPlane(DRMCard& card, const uint32_t id) noexcept : card{card}, id{id} {}

void DRMPlane::repaint(const DRMCRTC& crtc, DRMFramebuffer& fb, const int32_t x, const int32_t y) {
    try {
        const auto crtc_id {crtc.get_id()};

        const auto fb_id {fb.get_id()};
        const auto fb_w {fb.get_width()};
        const auto fb_h {fb.get_height()};
        const auto src_w {fb_w << 16};
        const auto src_h {fb_h << 16};

        if (card.are_atomic_commits_enabled()) {
            const DRMAtomicRequest req {card};
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "FB_ID", fb_id);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "CRTC_ID", crtc_id);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "CRTC_X", x);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "CRTC_Y", y);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "CRTC_W", fb_w);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "CRTC_H", fb_h);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "SRC_X", 0);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "SRC_Y", 0);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "SRC_W", src_w);
            req.add_property(id, DRM_MODE_OBJECT_PLANE, "SRC_H", src_h);

            req.commit(/*DRM_MODE_ATOMIC_NONBLOCK*/);
        } else {
            const auto res {drmModeSetPlane(card.get_fd(), id, crtc_id, fb_id, 0, x, y, fb_w, fb_h, 0, 0, src_w, src_h)};
            if (res == -EINVAL) {
                throw DRMException{"invalid plane id or CRTC id"};
            } else if (res < 0) {
                throw DRMException{errno};
            }
        }
    } catch (const DRMException& e) {
        throw DRMException{"failed to repaint plane framebuffer", e};
    }
}

DRMModePlaneUniquePtr DRMPlane::fetch_resource() const {
    // TODO: const? For all other local variables too
    DRMModePlaneUniquePtr plane {drmModeGetPlane(card.get_fd(), id), drmModeFreePlane};
    if (!plane) {
        throw DRMException{"cannot fetch plane", errno};
    }
    return plane;
}

// TODO: caching
bool DRMPlane::is_primary_plane() const {
    const DRMProperties props{card, *this};
    return props["type"] == DRM_PLANE_TYPE_PRIMARY;
}

bool DRMPlane::is_overlay_plane() const {
    const DRMProperties props{card, *this};
    return props["type"] == DRM_PLANE_TYPE_OVERLAY;
}

bool DRMPlane::is_cursor_plane() const {
    const DRMProperties props{card, *this};
    return props["type"] == DRM_PLANE_TYPE_CURSOR;
}

bool DRMPlane::is_compatible_with(const DRMCRTC& crtc) const {
    const auto crtc_bit {1 << crtc.get_index()};
    const auto possible_crtcs {get_possible_crtcs()};

    return (possible_crtcs & crtc_bit) != 0;
}

uint32_t DRMPlane::get_possible_crtcs() const {
    return fetch_resource()->possible_crtcs;
}

std::string DRMPlane::to_string() const noexcept {
    std::string s {"DRMPlane{"};
    s += "id=" + std::to_string(id);
    s += "}";
    return s;
}

}
