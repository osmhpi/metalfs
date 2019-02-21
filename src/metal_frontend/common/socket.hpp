#pragma once

#include <sys/socket.h>

#include <metal_frontend/messages/message_header.hpp>
#include <Messages.pb.h>
#include "../messages/message_header.hpp"
#include <exception>

#define ASSIGN_MESSAGE_TYPE(Type, Message) \
  template<> \
  void Socket::send_message<Type>(Message &message) { \
    _send_message(Type, message); \
  } \
  \
  template<> \
  Message Socket::receive_message<Type, Message>() { \
    return _receive_message<Type, Message>(); \
  }

namespace metal {

class Socket {
 public:
  explicit Socket(int fd) : _fd(fd) {}

  template<message_type TType, typename TMessage>
  void send_message(TMessage &message);

  template<message_type TType, typename TMessage>
  TMessage receive_message();

 protected:
  template<typename TMessage>
  void _send_message(message_type type, TMessage &message);

  template<message_type TType, typename TMessage>
  TMessage _receive_message();

  int _fd;
};

template<typename TMessage>
void Socket::_send_message(message_type type, TMessage &message) {
  char buf[message.ByteSize()];
  message.SerializeToArray(buf, message.ByteSize());

  MessageHeader header(type, message.ByteSize());
  header.sendHeader(_fd);
  send(_fd, buf, message.ByteSize(), 0);
}

template<message_type TType, typename TMessage>
TMessage Socket::_receive_message() {
  auto req = MessageHeader::receive(_fd);
  if (req.type() != TType)
    throw std::runtime_error("Unexpected message");

  char buffer [req.length()];
  recv(_fd, buffer, req.length(), 0);

  TMessage message;
  message.ParseFromArray(buffer, req.length());
  return message;
}

ASSIGN_MESSAGE_TYPE(message_type::SERVER_ACCEPT_AGENT, ServerAcceptAgent)
ASSIGN_MESSAGE_TYPE(message_type::AGENT_HELLO, ClientHello)

} // namespace metal
