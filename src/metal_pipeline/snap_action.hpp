#pragma once

#include <libsnap.h>

namespace metal {

class SnapAction {
public:
    explicit SnapAction(snap_action_type_t action_type, int card_no);
    SnapAction(const SnapAction &other) = delete;
    SnapAction(SnapAction &&other) noexcept;
    virtual ~SnapAction();

    void execute_job(uint64_t job_id, const char *parameters = nullptr, uint64_t direct_data_0 = 0, uint64_t direct_data_1 = 0, uint64_t *direct_data_out_0 = nullptr, uint64_t *direct_data_out_1 = nullptr);

protected:

    struct snap_action* _action;
    struct snap_card* _card;
};

} // namespace metal
