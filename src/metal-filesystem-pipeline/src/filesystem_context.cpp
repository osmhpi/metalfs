#include <metal-filesystem-pipeline/filesystem_context.hpp>

#include <dirent.h>
#include <sys/stat.h>

namespace metal {

FilesystemContext::FilesystemContext(
                                     std::string metadataDir,
                                     bool deleteMetadataIfExists)
    :  _context(nullptr) {
  DIR *dir = opendir(metadataDir.c_str());
  if (dir) {
    if (deleteMetadataIfExists) {
      auto dataFile = metadataDir + "/data.mdb";
      remove(dataFile.c_str());
      auto lockFile = metadataDir + "/lock.mdb";
      remove(lockFile.c_str());
    }
    closedir(dir);
  } else if (errno == ENOENT) {
    mkdir(metadataDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }
}

}  // namespace metal
