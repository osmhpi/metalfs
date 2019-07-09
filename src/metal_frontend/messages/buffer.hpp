#include <utility>

#pragma once

#include <stddef.h>
#include <string>

#define BUFFER_SIZE (1024 * 1024)

namespace metal {
class Buffer {
 public:
  Buffer(const Buffer &other) = delete;
  Buffer(Buffer &&other) noexcept : _filename(std::move(other._filename)), _file(other._file), _buffer(other._buffer), _current(other._current) {
      other._file = 0;
      other._buffer = nullptr;
  }
  Buffer& operator=(Buffer &&other) = default;

  static Buffer create_temp_file_for_shared_buffer(bool writable);
  static Buffer map_shared_buffer(std::string file_name, bool writable);

  virtual ~Buffer();

  void *current() { return reinterpret_cast<void*>(reinterpret_cast<char*>(_buffer) + (_current ? 1 : 0 * BUFFER_SIZE)); }
  void *next() { return reinterpret_cast<void*>(reinterpret_cast<char*>(_buffer) + (_current ? 0 : 1 * BUFFER_SIZE)); }
  void swap() { _current = !_current; }

  const std::string &filename() const { return _filename; }
  uint64_t size() { return BUFFER_SIZE; }

 protected:
  explicit Buffer(std::string filename, int file, void* buffer) : _filename(std::move(filename)), _file(file), _buffer(buffer), _current(false) {}

  std::string _filename;
  int _file;
  void *_buffer;
  bool _current;
};

} // namespace metal
