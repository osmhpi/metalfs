#pragma once

#include <libsnap.h>

namespace metal {

class SnapAction {
public:
    explicit SnapAction(snap_action_type_t action_type, int card_no);
    SnapAction(const SnapAction &other) = delete;
    SnapAction(SnapAction &&other) noexcept;
    virtual ~SnapAction();

    void execute_job(uint64_t job_id, char *parameters);

protected:

    struct snap_action* _action;
    struct snap_card* _card;
};

} // namespace metal
