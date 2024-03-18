#include "drm.h"
#include "../gui/gui.h"
#include <drm_fourcc.h>

namespace drm {

// TODO: make claim_unused_primary_plane atomic if threading is used
ScreenBitmap::ScreenBitmap() : crtc{find_crtc()}, plane{crtc.claim_unused_primary_plane()}, buffers{make_buffers()} {}

DRMCRTC& ScreenBitmap::find_crtc() {
    auto& card {gui::DisplayManager::the().get_drm_card()};
    return card.get_connected_crtc(); // TODO: select "primary" crtc?
}

std::array<std::unique_ptr<DRMFramebuffer>, 2> ScreenBitmap::make_buffers() const {
    auto& card {gui::DisplayManager::the().get_drm_card()}; // TODO: member variable?
    return std::array<std::unique_ptr<DRMFramebuffer>, 2>{
        std::make_unique<DRMFramebuffer>(card, plane, crtc.get_width(), crtc.get_height(), 32, DRM_FORMAT_ARGB8888),
        std::make_unique<DRMFramebuffer>(card, plane, crtc.get_width(), crtc.get_height(), 32, DRM_FORMAT_ARGB8888),
    };
}

void ScreenBitmap::render() {
    plane.repaint(crtc, *buffers[back], 0, 0);

    // TODO: do this here or in a separate refresh function?
    back ^= 1;
}

}
