#include <metal-pipeline/aws-f1/aws_f1_action.hpp>

#include <unistd.h>

#include <fpga_pci_sv.h>

// extern "C" {
// #include <fpga_pci.h>
// #include <fpga_mgmt.h>
// #include <utils/lcd.h>
// }

#include <utils/sh_dpi_tasks.h>

#include <iostream>
#include <stdexcept>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <metal-pipeline/fpga_interface.hpp>

/* Constants determined by the CL */
/* a set of register offsets; this CL has only one */
/* these register addresses should match the addresses in */
/* /aws-fpga/hdk/cl/examples/common/cl_common_defines.vh */
/* SV_TEST macro should be set if SW/HW co-simulation should be enabled */

#define HELLO_WORLD_REG_ADDR UINT64_C(0x500)
#define VLED_REG_ADDR	UINT64_C(0x504)

namespace metal {

std::unique_ptr<FpgaAction> AwsF1ActionFactory::createAction() const {
    return std::make_unique<AwsF1Action>(_card, _timeout);
}

AwsF1Action::AwsF1Action(int card, int timeout) : _timeout(timeout) {
  // spdlog::trace("Allocating CXL device /dev/cxl/afu{}.0s...", card);

  // char device[128];
  // snprintf(device, sizeof(device) - 1, "/dev/cxl/afu%d.0s", card);

  // _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
  // if (!_card) {
  //   throw std::runtime_error("Failed to open card");
  // }

  // spdlog::trace("Attaching to Metal FS action...");

  // auto action_irq =
  //     (snap_action_flag_t)(SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
  // _action = snap_attach_action(_card, fpga::ActionType, action_irq, 10);
  // if (!_action) {
  //   snap_card_free(_card);
  //   _card = nullptr;
  //   throw std::runtime_error("Failed to attach action");
  // }
}

AwsF1Action::AwsF1Action(AwsF1Action &&other) noexcept
    : _timeout(other._timeout) {}

AwsF1Action::~AwsF1Action() {
  // if (_action) {
  //   spdlog::trace("Detaching action...");
  //   snap_detach_action(_action);
  //   _action = nullptr;
  // }

  // if (_card) {
  //   spdlog::trace("Deallocating CXL device...");
  //   snap_card_free(_card);
  //   _card = nullptr;
  // }
}

void AwsF1Action::executeJob(fpga::JobType jobType, const void *parameters,
                            fpga::Address source, fpga::Address destination,
                            uint64_t directData0, uint64_t directData1,
                            uint64_t *directDataOut0,
                            uint64_t *directDataOut1) {
  // spdlog::debug("Starting job {}...", jobTypeToString(jobType));

  // fpga::Job mjob{};
  // mjob.job_type = jobType;
  // mjob.job_address = reinterpret_cast<uint64_t>(parameters);
  // mjob.source = source;
  // mjob.destination = destination;
  // mjob.direct_data[0] = directData0;
  // mjob.direct_data[1] = directData1;
  // mjob.direct_data[2] = 0;
  // mjob.direct_data[3] = 0;

  // struct snap_job cjob {};
  // snap_job_set(&cjob, &mjob, sizeof(mjob), nullptr, 0);

  // int rc = snap_action_sync_execute_job(_action, &cjob, _timeout);

  // if (rc != 0) {
  //   throw std::runtime_error("Error starting job: " +
  //                            snapReturnCodeToString(rc));
  // }

  // if (cjob.retc != SNAP_RETC_SUCCESS) {
  //   throw std::runtime_error("Job was unsuccessful");
  // }

  // if (directDataOut0) *directDataOut0 = mjob.direct_data[2];
  // if (directDataOut1) *directDataOut1 = mjob.direct_data[3];
}

bool AwsF1Action::isNVMeEnabled() {
  return false;
}

void *AwsF1Action::allocateMemory(size_t size) { return nullptr; }

std::string AwsF1Action::jobTypeToString(fpga::JobType job) {
  return "Unknown";
}

std::string AwsF1Action::addressTypeToString(fpga::AddressType addressType) {
  return "Unknown";
}

std::string AwsF1Action::mapTypeToString(fpga::MapType mapType) {
  return "Unknown";
}

std::string AwsF1Action::snapReturnCodeToString(int rc) {
  return "Unknown error";
}

}  // namespace metal
