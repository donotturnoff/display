#include "gui.h"
#include "drm.h"
#include <string>

namespace gui {

DisplayManager::DisplayManager(const std::string drm_card_path) : card{drm_card_path} {}

DisplayManager& DisplayManager::the() {
    static DisplayManager instance {"/dev/dri/card0"}; // TODO: allow this to be set. Make DM not a singleton?
    return instance;
}

drm::DRMCard& DisplayManager::get_drm_card() noexcept {
    return card;
}

}
