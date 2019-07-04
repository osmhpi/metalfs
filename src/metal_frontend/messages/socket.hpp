#pragma once

#include <sys/socket.h>

#include <metal_frontend/messages/message_header.hpp>
#include <Messages.pb.h>
#include "../messages/message_header.hpp"
#include <exception>

#define ASSIGN_MESSAGE_TYPE(Type, Message) \
  template<> \
  struct Socket::MessageTypeAssignment<Type> { \
    using type = Message; \
  };

namespace metal {

class Socket {
 public:
  explicit Socket(int fd) : _fd(fd) {}
  Socket(const Socket & other) = delete;
  Socket(Socket && other) noexcept : _fd(other._fd) { other._fd = 0; }

  template<message_type T>
  struct MessageTypeAssignment;

  template<message_type T>
  void send_message(typename MessageTypeAssignment<T>::type &message);

  template<message_type T>
  auto receive_message() -> typename MessageTypeAssignment<T>::type;

 protected:
  int _fd;
};

template<message_type T>
void Socket::send_message(typename Socket::MessageTypeAssignment<T>::type &message) {
  char buf[message.ByteSize()];
  message.SerializeToArray(buf, message.ByteSize());

  MessageHeader header(T, message.ByteSize());
  header.sendHeader(_fd);
  send(_fd, buf, message.ByteSize(), 0);
}

template<message_type T>
auto Socket::receive_message() -> typename Socket::MessageTypeAssignment<T>::type {
  auto req = MessageHeader::receive(_fd);
  if (req.type() != T)
    throw std::runtime_error("Unexpected message");

  char buffer [req.length()];
  recv(_fd, buffer, req.length(), 0);

  typename Socket::MessageTypeAssignment<T>::type message;
  message.ParseFromArray(buffer, req.length());
  return message;
}

ASSIGN_MESSAGE_TYPE(message_type::AgentHello, ClientHello)
ASSIGN_MESSAGE_TYPE(message_type::AgentPushBuffer, ClientPushBuffer)
ASSIGN_MESSAGE_TYPE(message_type::ServerAcceptAgent, ServerAcceptAgent)
ASSIGN_MESSAGE_TYPE(message_type::ServerProcessedBuffer, ServerProcessedBuffer)

} // namespace metal
