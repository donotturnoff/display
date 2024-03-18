#include "drm.h"

namespace drm {

// TODO: pixel format
MemBuffer::MemBuffer(const uint32_t width, const uint32_t height, const uint32_t bpp) :
        width{width}, height{height}, bpp{bpp} {
    buffer = alloc(); // TODO: do in initialiser list
}

MemBuffer::~MemBuffer() {
    delete buffer;
}

uint8_t* MemBuffer::alloc() const {
    // TODO: smart pointer?
    return new uint8_t[width*height*bpp/8];
}

}
