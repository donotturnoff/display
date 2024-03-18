#include "../style/style.h"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

#ifndef DRM_H
#define DRM_H

namespace gui {
class DisplayManager;
}

namespace drm {

// TODO: rename these to avoid confusion
// TODO: these add an extra word to the storage size of the unique_ptr
using DRMModeResDel = decltype(&drmModeFreeResources);
using DRMModeConnDel = decltype(&drmModeFreeConnector);
using DRMModeCRTCDel = decltype(&drmModeFreeCrtc);
using DRMModeEncoderDel = decltype(&drmModeFreeEncoder);
using DRMModePlaneResDel = decltype(&drmModeFreePlaneResources);
using DRMModePlaneDel = decltype(&drmModeFreePlane);

using DRMModeResUniquePtr = std::unique_ptr<drmModeRes, DRMModeResDel>;
using DRMModeConnUniquePtr = std::unique_ptr<drmModeConnector, DRMModeConnDel>;
using DRMModeCRTCUniquePtr = std::unique_ptr<drmModeCrtc, DRMModeCRTCDel>;
using DRMModeEncoderUniquePtr = std::unique_ptr<drmModeEncoder, DRMModeEncoderDel>;
using DRMModePlaneResUniquePtr = std::unique_ptr<drmModePlaneRes, DRMModePlaneResDel>;
using DRMModePlaneUniquePtr = std::unique_ptr<drmModePlane, DRMModePlaneDel>;

enum class BufferType {
    MEMORY, DRM_PRIMARY, DRM_CURSOR, DRM_OVERLAY
};

class DRMCard;
class DRMCRTC;
class DRMPlane;
class DRMFramebuffer;

// TODO: delete all copy constructors

class DRMConnector {
public:
    DRMConnector(DRMCard& card, const uint32_t id) noexcept;
    DRMConnector(const DRMConnector&) = delete;
    DRMConnector& operator=(const DRMConnector&) = delete;
    DRMCRTC& select_crtc() const;
    std::vector<drmModeModeInfo> fetch_modes() const;
    uint32_t get_id() const noexcept { return id; };
    bool is_connected() const;
    std::string to_string() const noexcept;
private:
    // TODO: move out
    DRMModeConnUniquePtr fetch_resource() const;
    std::vector<uint32_t> fetch_encoder_ids() const;

    DRMCard& card;
    const uint32_t id;
};

class DRMPlane {
public:
    DRMPlane(DRMCard& card, const uint32_t id) noexcept;
    DRMPlane(const DRMPlane&) = delete;
    DRMPlane& operator=(const DRMPlane&) = delete;
    void repaint(const DRMCRTC& crtc, DRMFramebuffer& fb, const int32_t x, const int32_t y);
    bool is_in_use() const noexcept { return in_use; }; // TODO: could a CRTC id ever be 0?
    void claim() { in_use = true; }; // TODO: lock usage?
    void release() { in_use = false; };
    bool is_primary_plane() const;
    bool is_cursor_plane() const;
    bool is_overlay_plane() const;
    bool is_compatible_with(const DRMCRTC& crtc) const;
    void set_pos(const uint32_t x, const uint32_t y) { this->x = x; this->y = y; }; // TODO: bounds check
    uint32_t get_id() const noexcept { return id; }
    std::string to_string() const noexcept;
private:
    DRMModePlaneUniquePtr fetch_resource() const;
    uint32_t get_possible_crtcs() const;

    DRMCard& card;
    bool in_use {false};
    const uint32_t id;
    uint32_t x {0};
    uint32_t y {0};
};

class DRMCRTC {
public:
    DRMCRTC(DRMCard& card, const uint32_t id, const uint32_t index) noexcept;
    DRMCRTC(const DRMCRTC&) = delete;
    DRMCRTC& operator=(const DRMCRTC&) = delete;
    uint32_t get_id() const noexcept { return id; };
    uint32_t get_index() const noexcept { return index; };
    void modeset(const DRMConnector& conn);
    DRMPlane& claim_unused_primary_plane() const;
    DRMPlane& claim_unused_cursor_plane() const;
    DRMPlane& claim_unused_overlay_plane() const;
    uint32_t get_width() const;
    uint32_t get_height() const;
    int32_t get_x() const;
    int32_t get_y() const;
    void add_connector(const DRMConnector& conn) noexcept;
    bool is_connected() const noexcept;
    std::string to_string() const noexcept;
private:
    DRMModeCRTCUniquePtr fetch_resource() const;

    // TODO: const (and fields in other classes)
    DRMCard& card;
    const uint32_t id, index;
    std::vector<uint32_t> connector_ids {};
};

class DRMEncoder {
public:
    DRMEncoder(DRMCard& card, const uint32_t id);
    DRMEncoder(const DRMEncoder&) = delete;
    DRMEncoder& operator=(const DRMEncoder&) = delete;
    DRMCRTC& select_crtc() const;
    std::string to_string() const noexcept;
private:
    DRMModeEncoderUniquePtr fetch_resource() const;
    uint32_t get_possible_crtcs() const { return fetch_resource()->possible_crtcs; };

    DRMCard& card;
    const uint32_t id;
};

class DRMCard {
public:
	DRMCard(const std::string& path);
    DRMCard(const DRMCard&) = delete;
    DRMCard& operator=(const DRMCard&) = delete;
	~DRMCard();
	int get_fd() const noexcept { return fd; };
	const std::vector<uint32_t>& get_crtc_ids() const noexcept { return crtc_ids; };
	void set_capabilities();
	void load_resources();
	void configure_connectors() noexcept;
	DRMCRTC& get_connected_crtc();
    DRMCRTC& get_crtc_by_id(const uint32_t id);
    DRMEncoder& get_encoder_by_id(const uint32_t id);
    DRMPlane& get_unused_overlay_plane(const DRMCRTC& crtc);
    DRMPlane& get_unused_primary_plane(const DRMCRTC& crtc); // TODO: use friend to limit access to DRMCRTC
    DRMPlane& get_unused_cursor_plane(const DRMCRTC& crtc);
    bool are_atomic_commits_enabled() const noexcept { return atomic_commits_enabled; };
private:
    int open_device(const std::string& path) const;
    uint64_t fetch_capability(const uint64_t capability) const;
    bool supports_async_page_flip() const;
    bool supports_dumb_buffers() const;
    bool supports_monotonic_timestamp() const;
    uint64_t max_cursor_width() const;
    uint64_t max_cursor_height() const;
    void enable_universal_planes();
    void enable_atomic_commits();

    const int fd;
    std::map<uint32_t, DRMConnector> connectors {};
    std::map<uint32_t, DRMCRTC> crtcs {};
    std::vector<uint32_t> crtc_ids {};
    std::map<uint32_t, DRMEncoder> encoders {};
    std::map<uint32_t, DRMPlane> planes {};
    std::vector<uint32_t> plane_ids {};

    bool atomic_commits_enabled {false};
};

class DRMException : public std::runtime_error {
public:
    DRMException(const std::string msg) noexcept;
    DRMException(const int errnum) noexcept;
    DRMException(const std::string msg, const int errnum) noexcept;
    DRMException(const std::string msg, const std::exception& e) noexcept;
    DRMException(const std::exception& e, const std::string extra) noexcept;
};

class DRMPropertyBlob {
public:
    DRMPropertyBlob(const DRMCard& card, const drmModeModeInfo* mode);
    DRMPropertyBlob(const DRMPropertyBlob&) = delete;
    DRMPropertyBlob& operator=(const DRMPropertyBlob&) = delete;
    ~DRMPropertyBlob();
    uint32_t get_id() const noexcept { return id; } // TODO: make other getters like this

private:
    const DRMCard& card;
    uint32_t id {0};
};

class DRMAtomicRequest {
public:
    DRMAtomicRequest(const DRMCard& card);
    DRMAtomicRequest(const DRMAtomicRequest&) = delete;
    DRMAtomicRequest& operator=(const DRMAtomicRequest&) = delete;
    ~DRMAtomicRequest();
	void add_property(const uint32_t obj_id, const uint32_t obj_type, const char* prop_name, const uint64_t val) const;
	void commit(const uint32_t flags) const;
    void commit() const;
private:
    const DRMCard& card;
    drmModeAtomicReq* req;
};

class DRMProperties {
public:
    DRMProperties(const DRMCard& card, const DRMPlane& plane);
    DRMProperties(const DRMProperties&) = delete;
    DRMProperties& operator=(const DRMProperties&) = delete;
    ~DRMProperties();
    uint64_t operator[](const std::string name) const;
private:
    const DRMCard& card;
    drmModeObjectProperties* props;
};

class Buffer {
public:
    virtual ~Buffer() {};
    uint8_t* get_buffer() noexcept { return buffer; };
    virtual uint32_t get_width() const noexcept = 0;
    virtual uint32_t get_height() const noexcept = 0;
    virtual uint32_t get_size() const noexcept = 0;
    void fill(const style::Colour c) const noexcept;
    void paint(Buffer& dst, const int32_t x, const int32_t y, bool over) const noexcept;
protected:
    uint8_t* buffer {nullptr};
private:
    void src_blend(const Buffer& dst, const uint32_t x, const uint32_t y, const uint32_t src_x, const uint32_t src_y, const uint32_t src_w, const uint32_t src_h) const noexcept;
    void src_over_blend(const Buffer& dst, const uint32_t x, const uint32_t y, const uint32_t src_x, const uint32_t src_y, const uint32_t src_w, const uint32_t src_h) const noexcept;
    uint32_t pixel_src_over(const uint32_t dst_v, const uint32_t src_v) const noexcept;
};

class MemBuffer : public Buffer {
public:
    MemBuffer(const uint32_t width, const uint32_t height, const uint32_t bpp);
    MemBuffer(const MemBuffer&) = delete;
    MemBuffer& operator=(const MemBuffer&) = delete;
    ~MemBuffer();
    uint32_t get_width() const noexcept { return width; };
    uint32_t get_height() const noexcept { return height; };
    uint32_t get_size() const noexcept { return width * height * 4; };
private:
    uint8_t* alloc() const;
    const uint32_t width, height, bpp;
};

class DRMFramebuffer : public Buffer {
public:
    DRMFramebuffer(const DRMCard& card, DRMPlane& plane, const uint32_t w, const uint32_t h,
        const uint32_t bpp, const uint32_t pixel_format);
    DRMFramebuffer(const DRMFramebuffer&) = delete;
    DRMFramebuffer& operator=(const DRMFramebuffer&) = delete;
    ~DRMFramebuffer();
    DRMPlane& get_plane() noexcept { return plane; };
    uint32_t get_id() const noexcept { return id; };
    uint32_t get_size() const noexcept { return info.size; }; // TODO: avoid wasted painting cycles outside of visible part of framebuffer
    uint32_t get_width() const noexcept { return info.width; };
    uint32_t get_height() const noexcept { return info.height; };
    void paint(DRMFramebuffer&, const int32_t, const int32_t, bool) const noexcept {};

private:
    void create_dumb_buffer();
    void add_framebuffer();
    void map_dumb_buffer();

    const DRMCard& card;
    DRMPlane& plane;
    drm_mode_create_dumb info;
    const uint32_t pixel_format;
    uint32_t id {0};
};

class ScreenBitmap {
public:
    ScreenBitmap();
    ScreenBitmap(const ScreenBitmap&) = delete;
    ScreenBitmap& operator=(const ScreenBitmap&) = delete;
    DRMFramebuffer* get_back_buffer() { return buffers[back].get(); };
    void fill(const style::Colour c) const noexcept { buffers[back]->fill(c); };
    DRMCRTC& get_crtc() { return crtc; };
    void render();
private:
    std::array<std::unique_ptr<DRMFramebuffer>, 2> make_buffers() const;
    DRMCRTC& find_crtc();

    int back {0};
    const uint32_t width {0}, height {0}; // TODO
    DRMCRTC& crtc;
    DRMPlane& plane;
    const std::array<std::unique_ptr<DRMFramebuffer>, 2> buffers;
};

class CursorBitmap {
public:
    CursorBitmap();
    CursorBitmap(const CursorBitmap&) = delete;
    CursorBitmap& operator=(const CursorBitmap&) = delete;
    DRMFramebuffer* get_back_buffer() { return buffers[back].get(); };
    DRMCRTC& get_crtc() { return crtc; };
    void render(const int32_t x, const int32_t y);
private:
    std::array<std::unique_ptr<DRMFramebuffer>, 2> make_buffers() const;
    DRMCRTC& find_crtc();

    int back {0};
    const uint32_t width {0}, height {0}; // TODO
    DRMCRTC& crtc;
    DRMPlane& plane;
    const std::array<std::unique_ptr<DRMFramebuffer>, 2> buffers;
};

class Bitmap {
public:
    Bitmap(const uint32_t width, const uint32_t height, const bool transparency = true, const bool hardware_backing = false);
    Bitmap(const Bitmap&) = delete;
    Bitmap& operator=(const Bitmap&) = delete;
    Buffer* get_back_buffer() { return buffers[back].get(); };
    void fill(const style::Colour c) const noexcept { buffers[back]->fill(c); };
    void render(Bitmap& target, const int32_t x, const int32_t y);
    void render(ScreenBitmap& target, const int32_t x, const int32_t y);
private:
    std::array<std::unique_ptr<Buffer>, 2> make_buffers() const;
    DRMPlane& find_plane(const DRMCRTC& crtc, const BufferType buffer_type) const;

    int back {0};
    const uint32_t width, height;
    const bool transparency, hardware_backing;
    const std::array<std::unique_ptr<Buffer>, 2> buffers;
};

}

#endif
