#include "drm.h"
#include <cstring>
#include <iostream>
#include <cstdio>
#include <sys/mman.h>
#include <xf86drmMode.h>
#include <xf86drm.h>

namespace drm {

// TODO: derive bpp from pixel_format
// TODO: reject attempts to make buffers that are too small (< 6x6? Determine from properties?)
DRMFramebuffer::DRMFramebuffer(const DRMCard& card, const uint32_t w, const uint32_t h,
        const uint32_t bpp, const uint32_t pixel_format) :
        card{card}, info{h, w, bpp, 0, 0, 0, 0}, pixel_format{pixel_format}
{
    const auto fd {card.get_fd()};

    create_dumb_buffer();

    try {
        add_framebuffer();

        try {
            map_dumb_buffer();
        } catch (const DRMException& e) {
            // TODO: separate function?
            if (drmModeRmFB(fd, id) < 0) {
                throw DRMException{e, "failed to remove framebuffer during cleanup"};
            }

            throw e;
        }
    } catch (const DRMException& e) {
        // TODO: refactor so that this can use the destructor (RAII)
        // TODO: separate function?
        struct drm_mode_destroy_dumb destroy_buf;
        destroy_buf.handle = info.handle;
        if (drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_buf) < 0) {
            throw DRMException{e, "failed to destroy dumb buffer during cleanup"};
        }

        throw e;
    }
}

DRMFramebuffer::~DRMFramebuffer() {
    const auto fd {card.get_fd()};

    if (drmModeRmFB(fd, id) < 0) {
        std::cerr << "failed to remove framebuffer" << std::endl;
    }

    struct drm_mode_destroy_dumb destroy_buf;
    destroy_buf.handle = info.handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_buf) < 0) {
        std::cerr << "failed to destroy dumb buffer" << std::endl;
    }
}

void DRMFramebuffer::create_dumb_buffer() {
    const auto fd {card.get_fd()};

    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &info) < 0) {
        throw DRMException{"failed to create dumb buffer", errno};
    }
}

void DRMFramebuffer::add_framebuffer() {
    const auto fd {card.get_fd()};

    const uint32_t handles[4] {info.handle, 0, 0, 0};
    const uint32_t pitches[4] {info.pitch, 0, 0, 0};
    const uint32_t offsets[4] {0, 0, 0, 0};

    /* Add dumb buffer as framebuffer */
    if (drmModeAddFB2(fd, info.width, info.height, pixel_format, handles, pitches, offsets, &id, 0) < 0) {
        throw DRMException{"failed to create framebuffer", errno};
    }
}

void DRMFramebuffer::map_dumb_buffer() {
    // TODO: don't have to do get_fd in every function
    const auto fd {card.get_fd()};

    /* Prepare dumb buffer for mapping */
    struct drm_mode_map_dumb map_buf {info.handle, 0, 0};
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_buf) < 0) {
        throw DRMException{"failed to prepare dumb buffer for mapping", errno};
    }

    /* Map dumb buffer into process address space */
    buffer = static_cast<uint8_t*>(mmap(nullptr, info.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_buf.offset));
    if (buffer == MAP_FAILED) {
        throw DRMException{"failed to map dumb buffer", errno};
    }
}

}
