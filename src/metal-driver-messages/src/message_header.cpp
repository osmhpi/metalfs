#include <netinet/in.h>
#include <sys/socket.h>

#include <metal-driver-messages/message_header.hpp>

namespace metal {

MessageHeader MessageHeader::receive(int socket) {
  MessageHeader incoming_message;
  recv(socket, &incoming_message, sizeof(incoming_message), 0);

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
