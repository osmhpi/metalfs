#pragma once

namespace metal {

enum class message_type : int {
  AGENT_HELLO,
  AGENT_PUSH_BUFFER,

  SERVER_ACCEPT_AGENT,
  SERVER_INITIALIZE_OUTPUT_BUFFER,
  SERVER_PROCESSED_BUFFER,
  SERVER_GOODBYE,

  ERROR
};

class MessageHeader {
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
