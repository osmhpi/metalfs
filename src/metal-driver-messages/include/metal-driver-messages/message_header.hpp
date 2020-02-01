#pragma once

#include <metal-driver-messages/metal-driver-messages_api.h>

#include <cstdint>

namespace metal {

enum class MessageType : int {
  RegistrationRequest,
  RegistrationResponse,

  ProcessingRequest,
  ProcessingResponse
};

class METAL_DRIVER_MESSAGES_API MessageHeader {
 public:
  MessageHeader() : MessageHeader(static_cast<MessageType>(0), 0) {}
  MessageHeader(MessageType type, int32_t length)
      : _type(static_cast<int>(type)), _length(length) {}

  MessageType type() { return static_cast<MessageType>(_type); }
  size_t length() { return static_cast<size_t>(_length); }

  static MessageHeader receive(int socket);
  void sendHeader(int socket);

 private:
  int _type;
  int32_t _length;
};

}  // namespace metal
