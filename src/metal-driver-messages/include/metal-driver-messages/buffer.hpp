#pragma once

#include <metal-driver-messages/metal-driver-messages_api.h>

#include <utility>

#include <stddef.h>
#include <string>

namespace metal {

const uint64_t BufferSize = 64 * 1024 * 1024;

class METAL_DRIVER_MESSAGES_API Buffer {
 public:
  Buffer(const Buffer &other) = delete;
  Buffer(Buffer &&other) noexcept
      : _filename(std::move(other._filename)),
        _file(other._file),
        _buffer(other._buffer),
        _current(other._current) {
    other._file = 0;
    other._buffer = nullptr;
  }
  Buffer &operator=(Buffer &&other) = default;

  static Buffer createTempFileForSharedBuffer(bool writable);
  static Buffer mapSharedBuffer(std::string file_name, bool writable);

  virtual ~Buffer();

  void *current() {
    return reinterpret_cast<void *>(reinterpret_cast<char *>(_buffer) +
                                    (_current ? BufferSize : 0));
  }
  void *next() {
    return reinterpret_cast<void *>(reinterpret_cast<char *>(_buffer) +
                                    (_current ? 0 : BufferSize));
  }
  void swap() { _current = !_current; }

  const std::string &filename() const { return _filename; }
  uint64_t size() { return BufferSize; }

 protected:
  explicit Buffer(std::string filename, int file, void *buffer)
      : _filename(std::move(filename)),
        _file(file),
        _buffer(buffer),
        _current(false) {}

  std::string _filename;
  int _file;
  void *_buffer;
  bool _current;
};

}  // namespace metal
