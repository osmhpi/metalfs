#pragma once

#include <libsnap.h>

namespace metal {

class SnapAction {
public:
    SnapAction(snap_action_type_t action_type, int card_no = 0);
    virtual ~SnapAction();

protected:

    struct snap_action* _action;
    struct snap_card* _card;
};

} // namespace metal
