#include "drm.h"
#include "../gui/gui.h"
#include <drm_fourcc.h>

namespace drm {

Bitmap::Bitmap(const uint32_t width, const uint32_t height, const bool transparency, const bool hardware_backing) :
    width{width}, height{height}, transparency{transparency}, hardware_backing{hardware_backing}, buffers{make_buffers()} {}

std::array<std::unique_ptr<Buffer>, 2> Bitmap::make_buffers() const {
    if (!hardware_backing) {
        return std::array<std::unique_ptr<Buffer>, 2>{
            std::make_unique<MemBuffer>(width, height, 32), // TODO allow bpp configuration
            std::make_unique<MemBuffer>(width, height, 32), // TODO allow bpp configuration
        };
    }

    auto& card {gui::DisplayManager::the().get_drm_card()};
    auto& crtc {card.get_connected_crtc()}; // TODO: select "primary" crtc?

    // TODO: make this atomic if threading is used
    DRMPlane& plane {crtc.claim_unused_overlay_plane()};

    return std::array<std::unique_ptr<Buffer>, 2>{
        std::make_unique<DRMFramebuffer>(card, plane, width, height, 32, DRM_FORMAT_ARGB8888),
        std::make_unique<DRMFramebuffer>(card, plane, width, height, 32, DRM_FORMAT_ARGB8888),
    };
}

void Bitmap::render(Bitmap& target, const int32_t x, const int32_t y) {
    // TODO: null check?

    auto& src {*buffers[back]};
    auto& dst {*target.buffers[target.back]};

    src.paint(dst, x, y, transparency);

    // TODO: do this here or in a separate refresh function?
    back ^= 1;
}

void Bitmap::render(ScreenBitmap& target, const int32_t x, const int32_t y) {
    // TODO: null check?

    auto& src {*buffers[back]};

    if (hardware_backing) {
        const DRMCRTC& crtc {target.get_crtc()};
        auto& drm_src {static_cast<DRMFramebuffer&>(src)};
        DRMPlane& src_plane {drm_src.get_plane()}; // TODO: yuck! separate Bitmap and HardwareBitmap?
        if (src_plane.is_compatible_with(crtc)) {
            target.render(); // TODO: necessary?
            src_plane.repaint(crtc, drm_src, x, y);
        } else {
            // TODO: do something! Choose new plane?
        }
    } else {
        DRMFramebuffer& dst {*target.get_back_buffer()};
        src.paint(dst, x, y, transparency);
        // TODO: target.render(); ? // TODO: copy new front buffer to back buffer?
    }

    // TODO: do this here or in a separate refresh function?
    back ^= 1;
}

}
