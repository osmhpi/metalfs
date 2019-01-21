#include "snap_action.hpp"

#include <libsnap.h>
#include <stdexcept>

namespace metal {

SnapAction::SnapAction(snap_action_type_t action_type, int card_no) {
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);

    _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
    if (!_card) {
        throw std::runtime_error("Failed to open card");
    }

    auto action_irq = (snap_action_flag_t)(SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
    _action = snap_attach_action(_card, action_type, action_irq, 30);
    if (!_action) {
        snap_card_free(_card);
        _card = nullptr;
        throw std::runtime_error("Failed to attach action");
    }
}

SnapAction::~SnapAction() {
    if (_action) {
        snap_detach_action(_action);
        _action = nullptr;
    }

    if (_card) {
        snap_card_free(_card);
        _card = nullptr;
    }
}

} // namespace metal
