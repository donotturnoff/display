#include "drm.h"
#include <drm_fourcc.h>
#include <iostream>

namespace drm {

DRMCRTC::DRMCRTC(DRMCard& card, const uint32_t id, const uint32_t index) noexcept : card{card}, id{id}, index{index},
    primary_plane{card.get_unused_primary_plane(*this)}, cursor_plane{card.get_unused_cursor_plane(*this)} // TODO: ensure using this here is safe
{
    primary_plane.claim_by(*this);
    cursor_plane.claim_by(*this);
}

bool DRMCRTC::is_connected() const noexcept {
    // TODO: make sure this is synchronised
    return connector_ids.size() > 0;
}

void DRMCRTC::modeset(const DRMConnector& conn) {
    try {
        // TODO: work out how adding subsequent connectors will work (e.g. don't modeset again, make sure they have the right modes)
        // TODO: only do this after successful modeset
        add_connector(conn);

        // TODO: allow configuration of this
        // Pick the first mode
        drmModeModeInfo mode {conn.fetch_modes()[0]};

        if (card.are_atomic_commits_enabled()) {
            const DRMPropertyBlob mode_blob {card, &mode};

            // Submit an atomic commit with ALLOW_MODESET
            const DRMAtomicRequest req {card};
            req.add_property(id, DRM_MODE_OBJECT_CRTC, "MODE_ID", mode_blob.get_id());
            req.add_property(id, DRM_MODE_OBJECT_CRTC, "ACTIVE", 1);

            for (const auto& conn_id: connector_ids) {
                req.add_property(conn_id, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", id);
            }

            //const uint32_t flags {DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_ATOMIC_ALLOW_MODESET};
            req.commit(/* flags */);
        } else {
            const auto res {drmModeSetCrtc(card.get_fd(), id, -1, 0, 0, connector_ids.data(), connector_ids.size(), &mode)};
            if (res == -1) {
                throw DRMException{"invalid list of connectors"};
            } else if (res == -EINVAL) {
                throw DRMException{"invalid CRTC id"};
            } else if (res < 0) {
                throw DRMException{errno};
            }
        }

        primary_plane.configure_framebuffers(get_width(), get_height(), 32, DRM_FORMAT_ARGB8888);

    } catch (const DRMException& e) {
        throw DRMException{"failed to modeset", e};
    }
}

DRMPlane& DRMCRTC::claim_overlay_plane() const {
    auto& plane {card.get_unused_overlay_plane(*this)};
    plane.claim_by(*this);
    return plane;
}


void DRMCRTC::add_connector(const DRMConnector& conn) noexcept {
    // TODO: make sure connector isn't connected elsewhere first
    connector_ids.push_back(conn.get_id());
}

DRMModeCRTCUniquePtr DRMCRTC::fetch_resource() const {
    DRMModeCRTCUniquePtr crtc {drmModeGetCrtc(card.get_fd(), id), drmModeFreeCrtc};
    if (!crtc) {
        throw DRMException{"cannot fetch CRTC resource", errno};
    }
    return crtc;
}

// TODO: prevent double call to fetch_resource (e.g. caching, or return width and height together)
uint32_t DRMCRTC::get_width() const {
    return fetch_resource()->width;
}

uint32_t DRMCRTC::get_height() const {
    return fetch_resource()->height;
}

// TODO: prevent double call to fetch_resource (e.g. caching, or return x and y together)
int32_t DRMCRTC::get_x() const {
    return fetch_resource()->x;
}

int32_t DRMCRTC::get_y() const {
    return fetch_resource()->y;
}

std::string DRMCRTC::to_string() const noexcept {
    std::string s {"DRMCRTC{"};
    s += "id=" + std::to_string(id);
    s += "}";
    return s;
}

}
