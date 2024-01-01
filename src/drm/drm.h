#include <array>
#include <cstdint>
#include <map>
#include <memory>
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

class DRMDevice {
public:
	DRMDevice();
	~DRMDevice();
};

class DRMCard;
class DRMCRTC;

class DRMCardFile {
public:
	DRMCardFile(const std::string& path);
	DRMCardFile(const DRMCardFile&) = delete; //TODO: does delete make sense for these?
	DRMCardFile(DRMCardFile&&) = delete;
	DRMCardFile& operator=(const DRMCardFile&) = delete;
	DRMCardFile& operator=(DRMCardFile&&) = delete;
	~DRMCardFile();
    int get_fd() const noexcept;
private:
	int open_device(const std::string& path) const;

    const int fd;
};

class DRMMode {
public:
    DRMMode(const drmModeModeInfo* mode) noexcept; // TODO: noexcept on other constructors
    DRMMode(const drmModeModeInfo& mode) noexcept; //TODO: avoid duplicating constructors
    std::string to_string() const noexcept;
private:
    const std::string name;
    const uint32_t clock;
    const uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
    const uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;
    const uint32_t vrefresh;
    const uint32_t flags;
    const uint32_t type;
};

class DRMConnector {
public:
    DRMConnector(DRMCard& card, const drmModeConnector* conn) noexcept;
    void select_crtc();
    std::string to_string() const noexcept;
private:
    // TODO: move out
    void fetch_modes(const int n, const drmModeModeInfo* modes) noexcept;
    void fetch_props(const int n, const uint32_t* props, const uint64_t* values) noexcept;
    void fetch_encoders(const int n, const uint32_t* encoders) noexcept;

    DRMCard& card;
    DRMCRTC* crtc {nullptr};
    const uint32_t id, encoder_id, type, type_id, mm_width, mm_height;
    const drmModeConnection connection; //TODO encapsulate this and one below?
    const drmModeSubPixel subpixel;
    std::vector<DRMMode> modes; // TODO: make const?
    std::map<uint32_t, uint64_t> props;
    std::vector<uint32_t> encoder_ids;

    static const std::array<std::string, 4> connection_strings;
    static const std::array<std::string, 7> subpixel_strings;
};

class DRMCRTC {
public:
    DRMCRTC(const drmModeCrtc* crtc) noexcept;
    std::string to_string() const noexcept;
    void add_connector(DRMConnector& conn) noexcept;
private:
    // TODO: just store drmModeCRTC field instead (and own it)
    // TODO: const (and fields in other classes)
    const uint32_t id, framebuffer_id; // framebuffer_id = FB id to connect, to 0 = disconnect
    const uint32_t x, y; // Position on the framebuffer
    const uint32_t width, height;
    const int mode_valid;
    const DRMMode mode;
    const int gamma_size; // Number of gamma stops TODO: should this be uint32_t?

    std::vector<DRMConnector&> connectors;
    std::vector<DRMPlane&> planes;
};

class DRMEncoder {
public:
    DRMEncoder(const drmModeEncoder* encoder) noexcept;
    uint32_t get_possible_crtcs() const noexcept;
    std::string to_string() const noexcept;
private:
    const uint32_t id, type, crtc_id, possible_crtcs, possible_clones;
};

class DRMPlane {
public:
    DRMPlane(const drmModePlane* plane) noexcept;
    std::string to_string() const noexcept;
private:
    void fetch_formats(const int n, const uint32_t* formats) noexcept;

    const uint32_t id;
    std::vector<uint32_t> formats;
    const uint32_t crtc_id, framebuffer_id;
    const uint32_t crtc_x, crtc_y;
    const uint32_t x, y;
    const uint32_t possible_crtcs;
    const uint32_t gamma_size;
};

class DRMFramebuffer {
public:
    DRMFramebuffer(const DRMCard& card, const uint32_t w, const uint32_t h,
        const uint32_t bpp, const uint32_t pixel_format);
private:
    void create_dumb_buffer();
    void add_framebuffer();
    void map_dumb_buffer();

    const int fd;
    drm_mode_create_dumb info;
    const uint32_t pixel_format;
    uint32_t id;
    uint32_t* buffer;
};

class DRMCard {
public:
	DRMCard(const std::string& path);
	int get_fd() const noexcept;
	std::vector<DRMCRTC>& get_crtcs() noexcept;
	std::vector<DRMEncoder>& get_encoders() noexcept;
	void configure_connectors() noexcept;
	void add_property() const;
private:
    uint64_t fetch_capability(const uint64_t capability) const;
    bool supports_async_page_flip() const;
    bool supports_dumb_buffers() const;
    bool supports_monotonic_timestamp() const;
    uint64_t max_cursor_width() const;
    uint64_t max_cursor_height() const;

    const DRMCardFile file;
    std::vector<DRMConnector> connectors; //TODO: replace with map id->connector? Ditto for others
    std::vector<DRMCRTC> crtcs;
    std::vector<DRMEncoder> encoders;
    std::vector<DRMPlane> planes;
};

class DRMException : public std::runtime_error {
public:
    DRMException(const std::string msg) noexcept;
    DRMException(const std::string msg, const int errnum) noexcept;
    DRMException(const std::string msg, const std::exception& e) noexcept;
    DRMException(const std::exception& e, const std::string extra) noexcept;
};

}

#endif
