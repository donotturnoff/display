#include "../drm/drm.h"
#include <string>

#ifndef GUI_H
#define GUI_H

namespace gui {

class DisplayManager {
public:
    DisplayManager(const std::string drm_card_path);
    static DisplayManager& the();
    drm::DRMCard& get_drm_card() noexcept;

private:
    drm::DRMCard card;

};

}

#endif
