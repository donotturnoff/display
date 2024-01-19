#include "drm/drm.h"
#include <iostream>
#include <drm_fourcc.h>
#include <cstring>
#include <cstdio>

int main(void) {
    drm::DRMCard card {"/dev/dri/card0"};

    // TODO: combine setup functions
    card.set_capabilities();
    card.load_resources();
    card.configure_connectors();

    // TODO: this isn't an ideal developer interface
    auto& crtc {card.get_connected_crtc()};
    auto& plane {crtc.claim_overlay_plane()};
    plane.configure_framebuffers(300, 200, 32, DRM_FORMAT_ARGB8888);
    plane.set_pos(200, 400);

    uint8_t* buf {plane.get_back_buffer().get_buffer()};
    for (uint32_t i {0}; i < plane.get_back_buffer().get_size(); i+=4) {
        buf[i] = 0xFF;
        buf[i+1] = 0xFF;
        //buf2[i+2] = 0xFF;
        buf[i+3] = 0xFF;
    }

    uint8_t* buf2 {crtc.get_back_buffer().get_buffer()};
    for (uint32_t i {0}; i < crtc.get_back_buffer().get_size(); i+=4) {
        buf2[i] = 0xFF;
        buf2[i+1] = 0x00;
        buf2[i+2] = 0x00;
        buf2[i+3] = 0xFF;
    }

    crtc.repaint();
    plane.repaint();

    auto& plane2 {crtc.claim_overlay_plane()};
    plane2.configure_framebuffers(50, 50, 32, DRM_FORMAT_ARGB8888);
    plane2.repaint();

    auto buf3 {plane2.get_back_buffer().get_buffer()};
    std::memset(buf3, 0x80, plane2.get_back_buffer().get_size());
    plane2.repaint();

    while(1);
}

