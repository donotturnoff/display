#include "drm.h"
#include <cassert>

namespace drm {

DRMConnector::DRMConnector(DRMCard& card, const drmModeConnector* conn) noexcept : card{card},
        id{conn->connector_id}, encoder_id{conn->encoder_id}, type{conn->connector_type},
        type_id{conn->connector_type_id}, mm_width{conn->mmWidth}, mm_height{conn->mmHeight},
        connection{conn->connection}, subpixel{conn->subpixel}
{
    fetch_modes(conn->count_modes, conn->modes);
    fetch_props(conn->count_props, conn->props, conn->prop_values);
    fetch_encoders(conn->count_encoders, conn->encoders);
}

DRMCRTC* DRMConnector::select_crtc() const {
    // Build a bitmask of all CRTCs we can use for this connector
    uint32_t possible_crtcs {0};
    for (const auto& enc: card.get_encoders()) {
        possible_crtcs |= enc.get_possible_crtcs();
    }

    auto& crtcs {card.get_crtcs()};
    for (auto i {0}; i < crtcs.size(); i++) {
        // Check the CRTC is compatible with the connector
        auto crtc_bit {1 << i};
        if ((possible_crtcs & crtc_bit) == 0) {
            continue;
        }
        return &crtcs[i];
    }

    throw DRMException{"cannot find suitable CRTC for connector #" + std::to_string(id)};
}

void DRMConnector::modeset(const DRMCRTC* crtc) {
    this->crtc = crtc;
    // TODO: store conn in crtc?

    auto fd {card.get_fd()};
    auto crtc_id {crtc->get_id()};

    // TODO: allow configuration of this
    // Pick the first mode
    auto& mode {modes[0]};

    // Create a blob for the mode
    uint32_t mode_id {0};
    drmModeCreatePropertyBlob(fd, mode, sizeof(*mode), &mode_id); // TODO: check error

    // Submit an atomic commit with ALLOW_MODESET
    auto req {drmModeAtomicAlloc()}; // TODO: wrap in unique_ptr
    card.add_property(req, plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_ID", crtc_id);
    card.add_property(req, crtc_id, DRM_MODE_OBJECT_CRTC, "MODE_ID", mode_id);
    card.add_property(req, crtc_id, DRM_MODE_OBJECT_CRTC, "active", 1);
    card.add_property(req, id, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", crtc_id);

    uint32_t flags {DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_ATOMIC_ALLOW_MODESET};
    auto res {drmModeAtomicCommit(fd, req, flags, nullptr)};
    if (res != 0) {
        drmModeAtomicFree();
        throw DRMException{"failed to modeset atomically"};
    }
    drmModeAtomicFree();
}

//TODO: return by value? move?
void DRMConnector::fetch_modes(const int n, const drmModeModeInfo* modes) noexcept {
    for (auto i {0}; i < n; i++) {
        this->modes.emplace_back(modes[i]);
    }
}

void DRMConnector::fetch_props(const int n, const uint32_t* props, const uint64_t* values) noexcept {
    for (auto i {0}; i < n; i++) {
        this->props[props[i]] = values[i];
    }
}

//TODO: replace uint32_t with a type alias
void DRMConnector::fetch_encoders(const int n, const uint32_t* encoder_ids) noexcept {
    for (auto i {0}; i < n; i++) {
        this->encoder_ids.emplace_back(encoder_ids[i]);
    }
}

const std::array<std::string, 4> DRMConnector::connection_strings {"", "connected", "disconnected", "unknown"};
const std::array<std::string, 7> DRMConnector::subpixel_strings {
    "", "unknown", "horizontal_rgb", "horizontal_bgr", "vertical_rgb", "vertical_bgr", "none"
};

// TODO: is to_string idiomatic?
std::string DRMConnector::to_string() const noexcept {
    std::string s {"DRMConnector{"};
    s += "id=" + std::to_string(id);
    s += ", encoder_id=" + std::to_string(encoder_id);
    s += ", type=" + std::to_string(type);
    s += ", type_id=" + std::to_string(type_id);
    s += ", mm_width=" + std::to_string(mm_width);
    s += ", mm_height=" + std::to_string(mm_height);
    s += ", connection=" + (connection_strings[connection]);
    s += ", subpixel=" + subpixel_strings[subpixel];

    s += ", modes=[";
    for (const auto& mode: modes) {
        s += mode.to_string() + ", ";
    }
    if (modes.size() > 0) { // TODO: yuck
        s.erase(s.end()-2, s.end());
    }
    s += "]";

    s += ", props={";
    for (const auto& [prop, value]: props) {
        s += std::to_string(prop) + "=" + std::to_string(value) + ", ";
    }
    if (props.size() > 0) {
        s.erase(s.end()-2, s.end());
    }
    s += "}";

    s += ", encoder_ids=[";
    for (const auto& encoder_id: encoder_ids) {
        s += std::to_string(encoder_id) + ", ";
    }
    if (encoder_ids.size() > 0) {
        s.erase(s.end()-2, s.end());
    }
    s += "]";

    s += "}";
    // TODO: add all properties
    return s;
}

}
