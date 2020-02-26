#include <metal-driver-messages/message_header.hpp>

#include <netinet/in.h>
#include <sys/socket.h>

#include <stdexcept>

namespace metal {

MessageHeader MessageHeader::receive(int socket) {
  MessageHeader incoming_message;
  auto bytes = recv(socket, &incoming_message, sizeof(incoming_message), 0);

  if (bytes != sizeof(incoming_message)) {
    throw std::runtime_error("Could not read message header from socket");
  }

  return MessageHeader(static_cast<MessageType>(ntohl(
                           static_cast<uint32_t>(incoming_message._type))),
                       ntohl(static_cast<uint32_t>(incoming_message._length)));
}

void MessageHeader::sendHeader(int socket) {
  MessageHeader outgoing_message(
      static_cast<MessageType>(htonl(static_cast<uint32_t>(_type))),
      htonl(static_cast<uint32_t>(_length)));

  send(socket, &outgoing_message, sizeof(outgoing_message), 0);
}

}  // namespace metal
