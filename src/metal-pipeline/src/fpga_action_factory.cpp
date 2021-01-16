#include <metal-pipeline/fpga_action_factory.hpp>

#include <metal-pipeline/ocaccel_action.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

std::unique_ptr<FpgaAction> SnapActionFactory::createAction() const {
    return std::make_unique<SnapAction>(_card, _timeout);
}

std::unique_ptr<FpgaAction> OCAccelActionFactory::createAction() const {
    return std::make_unique<OCAccelAction>(_card, _timeout);
}

}  // namespace metal
