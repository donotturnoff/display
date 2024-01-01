#include "drm.h"

namespace drm {

// TODO: int fd or DRMCard& card?
DRMPlane::DRMPlane(const DRMCard& card, const drmModePlane* plane) noexcept : card{card}, id{plane->plane_id},
        formats{}, crtc_id{plane->crtc_id}, framebuffer_id{plane->fb_id},
        crtc_x{plane->crtc_x}, crtc_y{plane->crtc_y}, x{plane->x}, y{plane->y}, // TODO: keep crtc_x, crtc_y etc synchronised. Also in other classes
        possible_crtcs{plane->possible_crtcs}, gamma_size{plane->gamma_size}, bufs{}, back{1}
{
    fetch_formats(plane->count_formats, plane->formats);
}

void DRMPlane::fetch_formats(const int n, const uint32_t* formats) noexcept {
    for (auto i {0}; i < n; i++) {
        this->formats.emplace_back(formats[i]);
    }
}

void DRMPlane::configure_framebuffers(const uint32_t w, const uint32_t h, const uint32_t bpp,
        const uint32_t pixel_format) {
    auto fd {card.get_fd()};

    fbs[0].emplace(fd, w, h, bpp, pixel_format);
    fbs[1].emplace(fd, w, h, bpp, pixel_format);
}

void DRMPlane::add_to_crtc(const DRMCRTC* crtc) {
    this->crtc = crtc;

    auto fd {card.get_fd()};
    auto crtc_id {crtc->get_id()};

    // TODO: allow configuration of this
    // Pick the first mode
    auto& mode {modes[0]};

    // Create a blob for the mode
    uint32_t mode_id {0};
    drmModeCreatePropertyBlob(fd, mode, sizeof(*mode), &mode_id); // TODO: check error

    // Submit an atomic commit with ALLOW_MODESET
    auto req {drmModeAtomicAlloc()}; // TODO: wrap in unique_ptr
    card.add_property(req, plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_ID", crtc_id);

    uint32_t flags {DRM_MODE_ATOMIC_NONBLOCK /* TODO: | DRM_MODE_ATOMIC_ALLOW_MODESET*/};
    auto res {drmModeAtomicCommit(fd, req, flags, nullptr)};
    if (res != 0) {
        drmModeAtomicFree();
        throw DRMException{"failed to add plane to CRTC atomically"};
    }
    drmModeAtomicFree();
}

// Atomically switches the back and front buffer
void DRMPlane::repaint() {
    const auto& fb {fbs[back]};
    const auto crtc_w {crtc->get_width()};
    const auto crtc_h {crtc->get_height()};

    auto req {drmModeAtomicAlloc()}; // TODO: wrap with unique_ptr
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "FB_ID", fb.get_id());
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "SRC_X", 0);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "SRC_Y", 0);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "SRC_W", fb.get_w() << 16);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "SRC_H", fb.get_h() << 16);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "CRTC_X", crtc_x);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "CRTC_Y", crtc_y);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "CRTC_W", crtc_w);
    card.add_property(req, id, DRM_MODE_OBJECT_PLANE, "CRTC_H", crtc_h);

    auto res {drmModeAtomicCommit(card.get_fd(), req, DRM_MODE_ATOMIC_NONBLOCK, nullptr)};
    if (res != 0) {
        drmModeAtomicFree();
        throw DRMException{"failed to display framebuffer atomically"};
    }
    drmModeAtomicFree();

    back ^= 1;

    // TODO: do I need to update the CRTC? Or does the change propagate automatically?
}

std::string DRMPlane::to_string() const noexcept {
    std::string s {"DRMPlane{"};
    s += "id=" + std::to_string(id);
    s += ", formats=[";
    for (const auto& format: formats) {
        s += std::to_string(format) + ", ";
    }
    if (formats.size() > 0) { // TODO: yuck
        s.erase(s.end()-2, s.end());
    }
    s += "]";

    s += ", crtc_id=" + std::to_string(crtc_id);
    s += ", framebuffer_id=" + std::to_string(framebuffer_id);
    s += ", crtc_x=" + std::to_string(crtc_x);
    s += ", crtc_y=" + std::to_string(crtc_y);
    s += ", x=" + std::to_string(x);
    s += ", y=" + std::to_string(y);
    s += ", possible_crtcs=" + std::to_string(possible_crtcs);
    s += ", gamma_size=" + std::to_string(gamma_size);
    s += "}";
    return s;
}

}
