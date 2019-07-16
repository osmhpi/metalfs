#pragma once

extern "C" {
#include <jv.h>
}

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "abstract_operator.hpp"
#include "operator_argument.hpp"

namespace metal {

class UserOperator : public AbstractOperator {

public:
    explicit UserOperator(std::string id, std::string manifest);
    virtual ~UserOperator();

    void configure(SnapAction& action) override;
    void finalize(SnapAction& action) override;

    std::string id() const override { return _id; }
    std::string description() const override;
    uint8_t internal_id() const override;
    bool needs_preparation() const override;
    virtual void set_is_prepared() override { _is_prepared = true; }
    bool prepare_required() const;

protected:
    void initializeOptions();

    jv manifest() const { return jv_copy(_manifest); }
    jv _manifest;

    std::string _id;
    bool _is_prepared;

};

} // namespace metal
