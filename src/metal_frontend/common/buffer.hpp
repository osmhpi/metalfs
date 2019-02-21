#pragma once

#include <stddef.h>
#include <string>

#define BUFFER_SIZE (1024 * 128)

namespace metal {
class Buffer {
 public:
  static Buffer create_temp_file_for_shared_buffer();
  static Buffer map_shared_buffer_for_reading(std::string file_name);

 protected:
  explicit Buffer(std::string filename, int file, void* buffer) : _filename(filename), _file(file), _buffer(buffer) {}

  void *_buffer;
  int _file;
  std::string _filename;
};

} // namespace metal
