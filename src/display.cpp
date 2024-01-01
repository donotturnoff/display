#include "drm/drm.h"

int main(void) {
    drm::DRMCard card {"/dev/dri/card0"};
    card.set_capabilities();
    card.load_resources();
    card.configure_connectors();
}
