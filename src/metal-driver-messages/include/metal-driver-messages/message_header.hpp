#pragma once

#include <metal-driver-messages/metal-driver-messages_api.h>

#include <cstdint>

namespace metal {

enum class message_type : int {
  AgentHello,
  AgentPushBuffer,

  ServerAcceptAgent,
  ServerProcessedBuffer
};

class METAL_DRIVER_MESSAGES_API MessageHeader {
 public:
  MessageHeader() : MessageHeader(static_cast<message_type>(0), 0) {}
  MessageHeader(message_type type, int32_t length) : _type(static_cast<int>(type)), _length(length) {}

  message_type type() { return static_cast<message_type>(_type); }
  size_t length() { return static_cast<size_t>(_length); }

  static MessageHeader receive(int socket);
  void sendHeader(int socket);

 private:
  int _type;
  int32_t _length;
};

} // namespace metal
