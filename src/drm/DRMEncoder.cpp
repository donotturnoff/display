#include "drm.h"

namespace drm {

DRMEncoder::DRMEncoder(const drmModeEncoder* encoder) noexcept : id{encoder->encoder_id}, type{encoder->encoder_type},
        crtc_id{encoder->crtc_id}, possible_crtcs{encoder->possible_crtcs}, possible_clones{encoder->possible_clones} {}

uint32_t DRMEncoder::get_possible_crtcs() const noexcept {
    return possible_crtcs;
}

std::string DRMEncoder::to_string() const noexcept {
    std::string s {"DRMEncoder{"};
    s += "id=" + std::to_string(id);
    s += ", type=" + std::to_string(type);
    s += ", crtc_id=" + std::to_string(crtc_id);
    s += ", possible_crtcs=" + std::to_string(possible_crtcs); // TODO: make more readable
    s += ", possible_clones=" + std::to_string(possible_clones);
    s += "}";
    return s;
}

}
