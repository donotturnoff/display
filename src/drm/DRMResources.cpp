#include "drm.h"

namespace drm {

DRMResources::DRMResources(const DRMCardFile& file) : resources{fetchResources(file)},
        crtcs{fetchCRTCs()}, encoders{fetchEncoders()}, connectors{fetchConnectors()}, planes{fetchPlanes()} {}

DRMResources::~DRMResources() {
    drmModeFreeResources(resources);
}

drmModeResPtr DRMResources::fetchResources(const DRMCardFile& file) const {
    auto resources {drmModeGetResources(file.getFd())};
    if (!resources) {
        throw DRMException{"cannot fetch resources for card", errno};
    }
    return resources;
}

std::vector<drmModeConnectorPtr> DRMResources::fetchConnectors() const {
    
    for (auto i {0}; i < res->count_connectors; i++) {
        auto c{drmModeGetConnector(fd, res->connectors[i])};
        if (!c) {
            throw DRMException{"cannot fetch connectors for card", errno};
        }
    }
}

}