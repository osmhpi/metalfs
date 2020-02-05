#pragma once

#include <metal-driver-messages/metal-driver-messages_api.h>

#include <sys/socket.h>

#include <exception>

#include <metal-driver-messages/messages.hpp>

#include "message_header.hpp"

#define ASSIGN_MESSAGE_TYPE(Type, Message)     \
  template <>                                  \
  struct Socket::MessageTypeAssignment<Type> { \
    using type = Message;                      \
  };

namespace metal {

class METAL_DRIVER_MESSAGES_API Socket {
 public:
  explicit Socket(int fd) : _fd(fd) {}
  Socket(const Socket &other) = delete;
  Socket(Socket &&other) noexcept : _fd(other._fd) { other._fd = 0; }

  template <MessageType T>
  struct MessageTypeAssignment;

  template <MessageType T>
  void sendMessage(typename MessageTypeAssignment<T>::type &message);

  template <MessageType T>
  auto receiveMessage() -> typename MessageTypeAssignment<T>::type;

 protected:
  int _fd;
};

template <MessageType T>
void Socket::sendMessage(
    typename Socket::MessageTypeAssignment<T>::type &message) {
  std::vector<char> buffer(message.ByteSize());
  message.SerializeToArray(buffer.data(), message.ByteSize());

  MessageHeader header(T, message.ByteSize());
  header.sendHeader(_fd);
  send(_fd, buffer.data(), message.ByteSize(), 0);
}

template <MessageType T>
auto Socket::receiveMessage() ->
    typename Socket::MessageTypeAssignment<T>::type {
  auto req = MessageHeader::receive(_fd);
  if (req.type() != T) throw std::runtime_error("Unexpected message");

  std::vector<char> buffer(req.length());
  recv(_fd, buffer.data(), req.length(), 0);

  typename Socket::MessageTypeAssignment<T>::type message;
  message.ParseFromArray(buffer.data(), req.length());
  return message;
}

ASSIGN_MESSAGE_TYPE(MessageType::RegistrationRequest, RegistrationRequest)
ASSIGN_MESSAGE_TYPE(MessageType::RegistrationResponse, RegistrationResponse)
ASSIGN_MESSAGE_TYPE(MessageType::ProcessingRequest, ProcessingRequest)
ASSIGN_MESSAGE_TYPE(MessageType::ProcessingResponse, ProcessingResponse)

}  // namespace metal
