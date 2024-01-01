#include "drm.h"

namespace drm {

DRMCRTC::DRMCRTC(const drmModeCrtc* crtc) noexcept : id{crtc->crtc_id}, framebuffer_id{crtc->buffer_id},
        x{crtc->x}, y{crtc->y}, width{crtc->width}, height{crtc->height}, mode_valid{crtc->mode_valid}, // TODO: what are x and y? How do I use them?
        mode{crtc->mode}, gamma_size{crtc->gamma_size} {}

std::string DRMCRTC::to_string() const noexcept {
    std::string s {"DRMCRTC{"};
    s += "id=" + std::to_string(id);
    s += ", framebuffer_id=" + std::to_string(framebuffer_id);
    s += ", x=" + std::to_string(x);
    s += ", y=" + std::to_string(y);
    s += ", width=" + std::to_string(width);
    s += ", height=" + std::to_string(height);
    s += ", mode_valid=" + std::to_string(mode_valid);
    s += ", mode=";
    if (mode_valid) {
        s += mode.to_string();
    } else {
        s += "invalid";
    }
    s += ", gamma_size=" + std::to_string(gamma_size);
    s += "}";
    return s;
}

}
