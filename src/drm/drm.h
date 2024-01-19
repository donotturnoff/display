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

class DRMCard;
class DRMCRTC;
class DRMPlane;

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

class DRMFramebuffer {
public:
    DRMFramebuffer(const DRMCard& card, const uint32_t w, const uint32_t h,
        const uint32_t bpp, const uint32_t pixel_format);
    DRMFramebuffer(const DRMFramebuffer&) = delete;
    DRMFramebuffer& operator=(const DRMFramebuffer&) = delete;
    ~DRMFramebuffer();
    uint32_t get_id() const noexcept { return id; };
    uint32_t get_width() const noexcept { return info.width; };
    uint32_t get_height() const noexcept { return info.height; };
    uint32_t get_size() const noexcept { return info.size; }; // TODO: avoid wasted painting cycles outside of visible part of framebuffer
    uint8_t* get_buffer() noexcept { return buffer; };

private:
    void create_dumb_buffer();
    void add_framebuffer();
    void map_dumb_buffer();

    const DRMCard& card;
    drm_mode_create_dumb info;
    const uint32_t pixel_format;
    uint32_t id {0};
    uint8_t* buffer {nullptr};
};

class DRMPlane {
public:
    DRMPlane(DRMCard& card, const uint32_t id) noexcept;
    DRMPlane(const DRMPlane&) = delete;
    DRMPlane& operator=(const DRMPlane&) = delete;
    void configure_framebuffers(const uint32_t w, const uint32_t h, const uint32_t bpp, const uint32_t pixel_format);
    void repaint();
    void claim_by(const DRMCRTC& crtc);
    DRMFramebuffer& get_back_buffer();
    bool is_in_use() const noexcept { return crtc_id > 0; }; // TODO: could a CRTC id ever be 0?
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
    const uint32_t id;
    uint32_t crtc_id {0}; // TODO: pointer?
    std::array<std::optional<DRMFramebuffer>, 2> fbs {};
    int back {0};
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
    void configure_framebuffers(const uint32_t bpp, const uint32_t pixel_format);
    void repaint() { primary_plane.repaint(); };
    DRMPlane& claim_overlay_plane() const;
    DRMFramebuffer& get_back_buffer() { return primary_plane.get_back_buffer(); };
    DRMModeCRTCUniquePtr fetch_resource() const;
    uint32_t get_width() const;
    uint32_t get_height() const;
    int32_t get_x() const;
    int32_t get_y() const;
    void add_connector(const DRMConnector& conn) noexcept;
    bool is_connected() const noexcept;
    std::string to_string() const noexcept;
private:
    // TODO: const (and fields in other classes)
    DRMCard& card;
    const uint32_t id, index;
    DRMPlane& primary_plane;
    DRMPlane& cursor_plane;
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

}

#endif
