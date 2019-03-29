#include "snap_action.hpp"

#include <libsnap.h>
#include <stdexcept>
#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include <iostream>

namespace metal {

SnapAction::SnapAction(snap_action_type_t action_type, int card_no) {
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);

    _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
    if (!_card) {
        throw std::runtime_error("Failed to open card");
    }

    auto action_irq = (snap_action_flag_t)(SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
    _action = snap_attach_action(_card, action_type, action_irq, 10);
    if (!_action) {
        snap_card_free(_card);
        _card = nullptr;
        throw std::runtime_error("Failed to attach action");
    }
}

SnapAction::SnapAction(SnapAction &&other) noexcept : _action(other._action), _card(other._card) {
    other._action = nullptr;
    other._card = nullptr;
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

void SnapAction::execute_job(uint64_t job_id, char *parameters) {

    metalfpga_job_t mjob;
    mjob.job_type = job_id;
    mjob.job_address = reinterpret_cast<uint64_t>(parameters);

    struct snap_job cjob{};
    snap_job_set(&cjob, &mjob, sizeof(mjob), nullptr, 0);

    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);

    if (rc != 0)
        throw std::runtime_error("Error starting job" + std::to_string(rc));

    if (cjob.retc != SNAP_RETC_SUCCESS)
        throw std::runtime_error("Job was unsuccessful");
}

} // namespace metal
