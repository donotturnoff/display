#include "drm.h"

namespace drm {

DRMEncoder::DRMEncoder(DRMCard& card, const uint32_t id) : card{card}, id{id} {}

DRMCRTC& DRMEncoder::select_crtc() const {
    // Use connected CRTC, if there is one
    const auto crtc_id {fetch_resource()->crtc_id};
    if (crtc_id) {
        return card.get_crtc_by_id(crtc_id);
    }

    const auto possible_crtcs {get_possible_crtcs()};

    const auto& crtc_ids {card.get_crtc_ids()};
    for (size_t i {0}; i < crtc_ids.size(); i++) {
        // Check the CRTC is compatible with the encoder
        const auto crtc_bit {1 << i};
        if ((possible_crtcs & crtc_bit) == 0) {
            continue;
        }
        return card.get_crtc_by_id(crtc_ids[i]);
    }

    throw DRMException{"cannot find suitable CRTC for encoder #" + std::to_string(id)};
}

DRMModeEncoderUniquePtr DRMEncoder::fetch_resource() const {
    DRMModeEncoderUniquePtr encoder {drmModeGetEncoder(card.get_fd(), id), drmModeFreeEncoder};
    if (!encoder) {
        throw DRMException{"cannot fetch encoder", errno};
    }
    return encoder;
}

std::string DRMEncoder::to_string() const noexcept {
    std::string s {"DRMEncoder{"};
    s += "id=" + std::to_string(id);
    s += "}";
    return s;
}

}
