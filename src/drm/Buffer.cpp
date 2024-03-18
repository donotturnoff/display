#include "drm.h"
#include <algorithm>
#include <cstring>

    #include <iostream>

namespace drm {

void Buffer::fill(const style::Colour c) const noexcept {
    // TODO: make buffer a uint32_t* by default
    uint32_t* buf {reinterpret_cast<uint32_t*>(buffer)};

    // Use memset to fill grayscale
    if (c.r == c.g && c.g == c.b && c.b == c.a) {
        std::memset(buf, c.r, get_size());
        return;
    }

    const uint32_t v {c.to_int()}; // TODO: better syntax for this?
    for (uint32_t i {0}; i < get_size()/4; i++) { // TODO: avoid overfilling DRMFramebuffers? (The buffer zones). Also avoid hardcoded /4
        buf[i] = v;
    }
}

void Buffer::paint(Buffer& dst, const int32_t x, const int32_t y, bool over) const noexcept {
    /* Clip source bitmap to destination bitmap */

    // TODO: fix negative x and y
    uint32_t clipped_x {std::min(static_cast<uint32_t>(std::max(x, 0)), dst.get_width())};
    uint32_t clipped_y {std::min(static_cast<uint32_t>(std::max(y, 0)), dst.get_height())};
    uint32_t clipped_src_x {clipped_x-x};
    uint32_t clipped_src_y {clipped_y-y};
    uint32_t clipped_src_w {std::min(get_width()-clipped_src_x, dst.get_width()-clipped_x)};
    uint32_t clipped_src_h {std::min(get_height()-clipped_src_y, dst.get_height()-clipped_y)};

    std::cerr << "clipped_y=" << clipped_y << " clipped_src_y=" << clipped_src_y << " clipped_src_h=" << clipped_src_h << " " << get_height() << std::endl;

    if (over) { // Blend with alpha
        src_over_blend(dst, clipped_x, clipped_y, clipped_src_x, clipped_src_y, clipped_src_w, clipped_src_h);
    } else {
        src_blend(dst, clipped_x, clipped_y, clipped_src_x, clipped_src_y, clipped_src_w, clipped_src_h);
    }
}

void Buffer::src_blend(const Buffer& dst, const uint32_t x, const uint32_t y, const uint32_t src_x, const uint32_t src_y, const uint32_t src_w, const uint32_t src_h) const noexcept {
    const auto src_buf_width {get_width()};
    const auto dst_buf_width {dst.get_width()};

    const uint32_t* src_buf {reinterpret_cast<uint32_t*>(buffer)};
    uint32_t* dst_buf {reinterpret_cast<uint32_t*>(dst.buffer)};

    const uint32_t size {src_w*4};
    for (uint32_t i {0}; i < src_h; i++) {
        uint32_t src_off {(src_y+i)*src_buf_width + src_x};
        uint32_t dst_off {(y+i)*dst_buf_width + x};
        std::memcpy(dst_buf + dst_off, src_buf + src_off, size);
    }
}

void Buffer::src_over_blend(const Buffer& dst, const uint32_t x, const uint32_t y, const uint32_t src_x, const uint32_t src_y, const uint32_t src_w, const uint32_t src_h) const noexcept {
    const auto src_buf_width {get_width()};
    const auto dst_buf_width {dst.get_width()};

    const uint32_t* src_buf {reinterpret_cast<uint32_t*>(buffer)};
    uint32_t* dst_buf {reinterpret_cast<uint32_t*>(dst.buffer)};

    std::cerr << ">> " << src_w << " " << src_h << " " << src_buf_width << std::endl;

    for (uint32_t i {0}; i < src_h; i++) {
        uint32_t src_off_base {(src_y+i)*src_buf_width + src_x};
        uint32_t dst_off_base {(y+i)*dst_buf_width + x};
        for (uint32_t j {0}; j < src_w; j++) {
            uint32_t src_off {src_off_base + j};
            uint32_t src_v {src_buf[src_off]};
            if (src_v <= 0xFFFFFF) continue; // Source is completely transparent

            uint32_t dst_off{dst_off_base + j};
            uint32_t dst_v;
            if (src_v >= 0xFF000000 || (dst_v = dst_buf[dst_off]) <= 0xFFFFFF) { // Source is opaque or destination is completely transparent
                dst_buf[dst_off] = src_v;
                continue;
            }

            dst_buf[dst_off] = style::Colour::src_over(src_v, dst_v);
        }
    }
}

}
