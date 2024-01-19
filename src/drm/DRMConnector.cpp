#include "drm.h"
#include <cassert>
#include <iostream>

namespace drm {

DRMConnector::DRMConnector(DRMCard& card, const uint32_t id) noexcept : card{card}, id{id} {}

DRMCRTC& DRMConnector::select_crtc() const {
    for (const auto& enc_id: fetch_encoder_ids()) {
        try {
            const auto& encoder {card.get_encoder_by_id(enc_id)};
            // TODO: set the encoder for this CRTC once we've found a CRTC
            return encoder.select_crtc();
        } catch (const DRMException& e) {
            std::cerr << "failed to select CRTC for connector #" << id << ": " << e.what() << std::endl;
        }
    }

    throw DRMException{"cannot find suitable CRTC for connector #" + std::to_string(id)};
}

DRMModeConnUniquePtr DRMConnector::fetch_resource() const {
    DRMModeConnUniquePtr conn {drmModeGetConnector(card.get_fd(), id), drmModeFreeConnector};
    if (!conn) {
        throw DRMException{"cannot fetch connector resource", errno};
    }
    return conn;
}

bool DRMConnector::is_connected() const {
    const auto conn {fetch_resource()};
    return conn->connection == DRM_MODE_CONNECTED;
}

//TODO: return by value? move?
std::vector<drmModeModeInfo> DRMConnector::fetch_modes() const {
    const auto conn {fetch_resource()};

    std::vector<drmModeModeInfo> modes;
    for (auto i {0}; i < conn->count_modes; i++) {
        modes.push_back(conn->modes[i]);
    }
    return modes;
}

//TODO: replace uint32_t with a type alias
std::vector<uint32_t> DRMConnector::fetch_encoder_ids() const {
    const auto conn {fetch_resource()}; // TODO: avoid duplicating work between calls to fetch_modes and fetch_encoder_ids

    std::vector<uint32_t> encoder_ids;
    for (int i {0}; i < conn->count_encoders; i++) {
        encoder_ids.emplace_back(conn->encoders[i]);
    }
    return encoder_ids;
}

// TODO: is to_string idiomatic?
std::string DRMConnector::to_string() const noexcept {
    std::string s {"DRMConnector{"};
    s += "id=" + std::to_string(id);
    s += "}";
    // TODO: add all properties
    return s;
}

}
