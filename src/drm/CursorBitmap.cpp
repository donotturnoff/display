#include "drm.h"
#include "../gui/gui.h"
#include <drm_fourcc.h>

namespace drm {

// TODO: make claim_unused_primary_plane atomic if threading is used
CursorBitmap::CursorBitmap() : crtc{find_crtc()}, plane{crtc.claim_unused_cursor_plane()}, buffers{make_buffers()} {}

DRMCRTC& CursorBitmap::find_crtc() {
    auto& card {gui::DisplayManager::the().get_drm_card()};
    return card.get_connected_crtc(); // TODO: select "primary" crtc?
}

std::array<std::unique_ptr<DRMFramebuffer>, 2> CursorBitmap::make_buffers() const {
    auto& card {gui::DisplayManager::the().get_drm_card()};
    return std::array<std::unique_ptr<DRMFramebuffer>, 2>{
        std::make_unique<DRMFramebuffer>(card, plane, width, height, 32, DRM_FORMAT_ARGB8888),
        std::make_unique<DRMFramebuffer>(card, plane, width, height, 32, DRM_FORMAT_ARGB8888),
    };
}

void CursorBitmap::render(const int32_t x, const int32_t y) {
    plane.repaint(crtc, *buffers[back], x, y);
    back ^= 1;
}

}
