#pragma once

#include <stddef.h>
#include <string>

#define BUFFER_SIZE (1024 * 128)

namespace metal {
class Buffer {
 public:
  static Buffer create_temp_file_for_shared_buffer(bool writable);
  static Buffer map_shared_buffer(std::string file_name, bool writable);
  virtual ~Buffer();

  void *buffer() { return _buffer; }
  const std::string &filename() const { return _filename; }
  uint64_t size() { return BUFFER_SIZE; }

 protected:
  explicit Buffer(std::string filename, int file, void* buffer) : _filename(filename), _file(file), _buffer(buffer) {}

  void *_buffer;
  int _file;
  std::string _filename;
};

} // namespace metal
