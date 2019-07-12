#pragma once

#include <libsnap.h>

#include <string>

#include <metal_fpga/hw/hls/include/snap_action_metal.h>

namespace metal {

class SnapAction {
public:
    explicit SnapAction(snap_action_type_t action_type, int card_no);
    SnapAction(const SnapAction &other) = delete;
    SnapAction(SnapAction &&other) noexcept;
    virtual ~SnapAction();

    void execute_job(fpga::JobType job_type, const void *parameters = nullptr, fpga::Address source = {}, fpga::Address destination = {}, uint64_t direct_data_0 = 0, uint64_t direct_data_1 = 0, uint64_t *direct_data_out_0 = nullptr, uint64_t *direct_data_out_1 = nullptr);
    bool is_nvme_enabled();

protected:
    std::string job_type_to_string(fpga::JobType job);
    std::string snap_return_code_to_string(int rc);

    struct snap_action* _action;
    struct snap_card* _card;
};

} // namespace metal
