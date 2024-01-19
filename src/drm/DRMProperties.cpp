#include "drm.h"

namespace drm {

DRMProperties::DRMProperties(const DRMCard& card, const DRMPlane& plane) : card{card},
    props{drmModeObjectGetProperties(card.get_fd(), plane.get_id(), DRM_MODE_OBJECT_PLANE)} {}

DRMProperties::~DRMProperties() {
    drmModeFreeObjectProperties(props);
}

uint64_t DRMProperties::operator[](const std::string name) const {
	for (uint32_t i {0}; i < props->count_props; i++) {
		const auto prop {drmModeGetProperty(card.get_fd(), props->props[i])}; // TODO: wrap in DRMProperty class
		if (prop->name == name) {
            drmModeFreeProperty(prop);
			return props->prop_values[i];
		}
        drmModeFreeProperty(prop);
	}

    throw DRMException{"property " + name + " does not exist"};
}

}
