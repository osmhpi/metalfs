#include <utility>

#pragma once

#include <stddef.h>
#include <string>

#define BUFFER_SIZE (1024 * 128)

namespace metal {
class Buffer {
 public:
  Buffer(const Buffer &other) = delete;
  Buffer(Buffer &&other) noexcept : _filename(std::move(other._filename)), _file(other._file), _buffer(other._buffer) {
      other._file = 0;
      other._buffer = nullptr;
  }
  Buffer& operator=(Buffer &&other) = default;

  static Buffer create_temp_file_for_shared_buffer(bool writable);
  static Buffer map_shared_buffer(std::string file_name, bool writable);
  virtual ~Buffer();

  void *buffer() { return _buffer; }
  const std::string &filename() const { return _filename; }
  uint64_t size() { return BUFFER_SIZE; }

 protected:
  explicit Buffer(std::string filename, int file, void* buffer) : _filename(std::move(filename)), _file(file), _buffer(buffer) {}

  std::string _filename;
  int _file;
  void *_buffer;
};

} // namespace metal
