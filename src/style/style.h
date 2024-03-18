#include <cstdint>
#include <algorithm>

#ifndef STYLE_H
#define STYLE_H

namespace style {


// Colour stores RGB+A colour information, with a pre-multiplied alpha value
// TODO: move some of this to Colour.cpp
// TODO: support other colour formats than just ARGB
struct Colour {
    constexpr Colour(const uint8_t r = 0, const uint8_t g = 0, const uint8_t b = 0, const uint8_t a = 0xFF) :
        r{static_cast<uint8_t>((r*a)/0xFF)},
        g{static_cast<uint8_t>((g*a)/0xFF)},
        b{static_cast<uint8_t>((b*a)/0xFF)}, // TODO: use () for implicit narrowing?
        a{a} {};

    constexpr Colour(const uint32_t v) :
        r{static_cast<uint8_t>(v>>16)},
        g{static_cast<uint8_t>(v>>8)},
        b{static_cast<uint8_t>(v)},
        a{static_cast<uint8_t>(v>>24)} {};

    static constexpr Colour black(const uint8_t a = 0xFF)   { return Colour{0x00, 0x00, 0x00, a}; }
    static constexpr Colour blue(const uint8_t a = 0xFF)    { return Colour{0x00, 0x00, 0xFF, a}; }
    static constexpr Colour green(const uint8_t a = 0xFF)   { return Colour{0x00, 0xFF, 0x00, a}; }
    static constexpr Colour cyan(const uint8_t a = 0xFF)    { return Colour{0x00, 0xFF, 0xFF, a}; }
    static constexpr Colour red(const uint8_t a = 0xFF)     { return Colour{0xFF, 0x00, 0x00, a}; }
    static constexpr Colour magenta(const uint8_t a = 0xFF) { return Colour{0xFF, 0x00, 0xFF, a}; }
    static constexpr Colour yellow(const uint8_t a = 0xFF)  { return Colour{0xFF, 0xFF, 0x00, a}; }
    static constexpr Colour white(const uint8_t a = 0xFF)   { return Colour{0xFF, 0xFF, 0xFF, a}; }
    static constexpr Colour grey(const uint8_t a = 0xFF)    { return Colour{0x7F, 0x7F, 0x7F, a}; }
    static constexpr Colour clear()                         { return Colour(0x00, 0x00, 0x00, 0x00); }

    inline uint32_t to_int() const noexcept;

    inline Colour lighten(const uint8_t shift) const noexcept;

    static inline uint32_t src_over(const uint32_t src_v, const uint32_t dst_v) noexcept;

    const uint8_t r, g, b, a;
};

uint32_t Colour::to_int() const noexcept {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

Colour Colour::lighten(const uint8_t shift) const noexcept {
    // TODO: use C++-style casts
    const uint8_t new_r {static_cast<uint8_t>(std::min(0xFF, static_cast<int>(r + shift)))};
    const uint8_t new_g {static_cast<uint8_t>(std::min(0xFF, static_cast<int>(g + shift)))};
    const uint8_t new_b {static_cast<uint8_t>(std::min(0xFF, static_cast<int>(b + shift)))};

    return Colour{new_r, new_g, new_b, a};
}

// TODO: make use of vector operations
uint32_t Colour::src_over(const uint32_t src_v, const uint32_t dst_v) noexcept {
    const uint32_t src_a {src_v >> 24};
    const uint32_t src_r {(src_v >> 16) & 0xFF};
    const uint32_t src_g {(src_v >> 8) & 0xFF};
    const uint32_t src_b {src_v & 0xFF};

    const uint32_t dst_a {dst_v >> 24};
    const uint32_t dst_r {(dst_v >> 16) & 0xFF};
    const uint32_t dst_g {(dst_v >> 8) & 0xFF};
    const uint32_t dst_b {dst_v & 0xFF};

    const uint32_t p {0xFF - src_a};

    const uint32_t a {src_a + ((dst_a*p)>>8)};
    const uint32_t r {src_r + ((dst_r*p)>>8)};
    const uint32_t g {src_g + ((dst_g*p)>>8)};
    const uint32_t b {src_b + ((dst_b*p)>>8)};

    return (a << 24) | (r << 16) | (g << 8) | b;
}

}

#endif
