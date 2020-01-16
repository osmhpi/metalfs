#pragma once

#include <metal-driver-messages/metal-driver-messages_api.h>

#include <sys/socket.h>

#include <exception>
#include <metal-driver-messages/messages.hpp>
#include "message_header.hpp"

#define ASSIGN_MESSAGE_TYPE(Type, Message) \
  template<> \
  struct Socket::MessageTypeAssignment<Type> { \
    using type = Message; \
  };

namespace metal {

class METAL_DRIVER_MESSAGES_API Socket {
 public:
  explicit Socket(int fd) : _fd(fd) {}
  Socket(const Socket & other) = delete;
  Socket(Socket && other) noexcept : _fd(other._fd) { other._fd = 0; }

  template<message_type T>
  struct MessageTypeAssignment;

  template<message_type T>
  void send_message(typename MessageTypeAssignment<T>::type &message);

  template<message_type T>
  auto receiveMessage() -> typename MessageTypeAssignment<T>::type;

 protected:
  int _fd;
};

template<message_type T>
void Socket::send_message(typename Socket::MessageTypeAssignment<T>::type &message) {
  std::vector<char> buffer(message.ByteSize());
  message.SerializeToArray(buffer.data(), message.ByteSize());

  MessageHeader header(T, message.ByteSize());
  header.sendHeader(_fd);
  send(_fd, buffer.data(), message.ByteSize(), 0);
}

template<message_type T>
auto Socket::receiveMessage() -> typename Socket::MessageTypeAssignment<T>::type {
  auto req = MessageHeader::receive(_fd);
  if (req.type() != T)
    throw std::runtime_error("Unexpected message");

  std::vector<char> buffer(req.length());
  recv(_fd, buffer.data(), req.length(), 0);

  typename Socket::MessageTypeAssignment<T>::type message;
  message.ParseFromArray(buffer.data(), req.length());
  return message;
}

ASSIGN_MESSAGE_TYPE(message_type::RegistrationRequest, RegistrationRequest)
ASSIGN_MESSAGE_TYPE(message_type::RegistrationResponse, RegistrationResponse)
ASSIGN_MESSAGE_TYPE(message_type::ProcessingRequest, ProcessingRequest)
ASSIGN_MESSAGE_TYPE(message_type::ProcessingResponse, ProcessingResponse)

} // namespace metal
