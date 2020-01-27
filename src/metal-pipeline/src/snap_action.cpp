#include <metal-pipeline/snap_action.hpp>

#include <unistd.h>

#include <libsnap.h>
#include <snap_hls_if.h>

#include <iostream>
#include <stdexcept>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <snap_action_metal.h>

namespace metal {

SnapAction::SnapAction(int card) {
  spdlog::trace("Allocating CXL device /dev/cxl/afu{}.0s...", card);

  char device[128];
  snprintf(device, sizeof(device) - 1, "/dev/cxl/afu%d.0s", card);

  _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
  if (!_card) {
    throw std::runtime_error("Failed to open card");
  }

  spdlog::trace("Attaching to Metal FS action...");

  auto action_irq =
      (snap_action_flag_t)(SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
  _action = snap_attach_action(_card, fpga::ActionType, action_irq, 10);
  if (!_action) {
    snap_card_free(_card);
    _card = nullptr;
    throw std::runtime_error("Failed to attach action");
  }
}

SnapAction::SnapAction(SnapAction &&other) noexcept
    : _action(other._action), _card(other._card) {
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

void SnapAction::execute_job(fpga::JobType job_type, const void *parameters,
                             fpga::Address source, fpga::Address destination,
                             uint64_t direct_data_0, uint64_t direct_data_1,
                             uint64_t *direct_data_out_0,
                             uint64_t *direct_data_out_1) {
  spdlog::debug("Starting job {}...", job_type_to_string(job_type));

  fpga::Job mjob;
  mjob.job_type = job_type;
  mjob.job_address = reinterpret_cast<uint64_t>(parameters);
  mjob.source = source;
  mjob.destination = destination;
  mjob.direct_data[0] = direct_data_0;
  mjob.direct_data[1] = direct_data_1;
  mjob.direct_data[2] = 0;
  mjob.direct_data[3] = 0;

  struct snap_job cjob {};
  snap_job_set(&cjob, &mjob, sizeof(mjob), nullptr, 0);

  const unsigned long timeout = 30;
  int rc = snap_action_sync_execute_job(_action, &cjob, timeout);

  if (rc != 0)
    throw std::runtime_error("Error starting job: " +
                             snap_return_code_to_string(rc));

  if (cjob.retc != SNAP_RETC_SUCCESS)
    throw std::runtime_error("Job was unsuccessful");

  if (direct_data_out_0) *direct_data_out_0 = mjob.direct_data[2];
  if (direct_data_out_1) *direct_data_out_1 = mjob.direct_data[3];
}

bool SnapAction::is_nvme_enabled() {
  unsigned long have_nvme = 0;
  snap_card_ioctl(_card, GET_NVME_ENABLED, (unsigned long)&have_nvme);
  return have_nvme != 0;
}

void *SnapAction::allocateMemory(size_t size) { return snap_malloc(size); }

std::string SnapAction::job_type_to_string(fpga::JobType job) {
  switch (job) {
    case fpga::JobType::ReadImageInfo:
      return "ReadImageInfo";
    case fpga::JobType::Map:
      return "Map";
    case fpga::JobType::ConfigureStreams:
      return "ConfigureStreams";
    case fpga::JobType::ResetPerfmon:
      return "ResetPerfmon";
    case fpga::JobType::ConfigurePerfmon:
      return "ConfigurePerfmon";
    case fpga::JobType::ReadPerfmonCounters:
      return "ReadPerfmonCounters";
    case fpga::JobType::RunOperators:
      return "RunOperators";
    case fpga::JobType::ConfigureOperator:
      return "ConfigureOperator";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string SnapAction::address_type_to_string(fpga::AddressType addressType) {
  switch (addressType) {
    case fpga::AddressType::Host:
      return "Host";
    case fpga::AddressType::CardDRAM:
      return "CardDRAM";
    case fpga::AddressType::NVMe:
      return "NVMe";
    case fpga::AddressType::Null:
      return "Null";
    case fpga::AddressType::Random:
      return "Random";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string SnapAction::map_type_to_string(fpga::MapType mapType) {
  switch (mapType) {
    case fpga::MapType::None:
      return "None";
    case fpga::MapType::DRAM:
      return "DRAM";
    case fpga::MapType::NVMe:
      return "NVMe";
    case fpga::MapType::DRAMAndNVMe:
      return "DRAMAndNVMe";
  }

  // The compiler should treat unhandled enum values as an error, so we should
  // never end up here
  return "Unknown";
}

std::string SnapAction::snap_return_code_to_string(int rc) {
  switch (rc) {
    case SNAP_OK:
      return "";
    case SNAP_EBUSY:
      return "Resource is busy";
    case SNAP_ENODEV:
      return "No such device";
    case SNAP_EIO:
      return "Problem accessing the card";
    case SNAP_ENOENT:
      return "Entry not found";
    case SNAP_EFAULT:
      return "Illegal address";
    case SNAP_ETIMEDOUT:
      return "Timeout error";
    case SNAP_EINVAL:
      return "Invalid parameters";
    case SNAP_EATTACH:
      return "Attach error";
    case SNAP_EDETACH:
      return "Detach error";
    default:
      return "Unknown error";
  }
}

}  // namespace metal
