#include "drm.h"

namespace drm {

DRMMode::DRMMode(const drmModeModeInfo* mode) noexcept : name{mode->name}, clock{mode->clock},
        hdisplay{mode->hdisplay}, hsync_start{mode->hsync_start}, hsync_end{mode->hsync_end},
        htotal{mode->htotal}, hskew{mode->hskew},
        vdisplay{mode->vdisplay}, vsync_start{mode->vsync_start}, vsync_end{mode->vsync_end},
        vtotal{mode->vtotal}, vscan{mode->vscan},
        vrefresh{mode->vrefresh}, flags{mode->flags}, type{mode->type} {}

DRMMode::DRMMode(const drmModeModeInfo& mode) noexcept : name{mode.name}, clock{mode.clock},
        hdisplay{mode.hdisplay}, hsync_start{mode.hsync_start}, hsync_end{mode.hsync_end},
        htotal{mode.htotal}, hskew{mode.hskew},
        vdisplay{mode.vdisplay}, vsync_start{mode.vsync_start}, vsync_end{mode.vsync_end},
        vtotal{mode.vtotal}, vscan{mode.vscan},
        vrefresh{mode.vrefresh}, flags{mode.flags}, type{mode.type} {}

std::string DRMMode::to_string() const noexcept {
    std::string s {"DRMMode{"};
    s += "name=" + std::string(name);
    s += ", clock=" + std::to_string(clock);
    s += ", hdisplay=" + std::to_string(hdisplay);
    s += ", vdisplay=" + std::to_string(vdisplay);
    s += ", hsync_start=" + std::to_string(hsync_start);
    s += ", vsync_start=" + std::to_string(vsync_start);
    s += ", hsync_end=" + std::to_string(hsync_end);
    s += ", vsync_end=" + std::to_string(vsync_end);
    s += ", htotal=" + std::to_string(htotal);
    s += ", vtotal=" + std::to_string(vtotal);
    s += ", hskew=" + std::to_string(hskew);
    s += ", vscan=" + std::to_string(vscan);
    s += ", vrefresh=" + std::to_string(vrefresh);
    s += ", flags=" + std::to_string(flags);
    s += ", type=" + std::to_string(type);
    s += "}";
    return s;
}

}
