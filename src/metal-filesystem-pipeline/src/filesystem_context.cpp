#include <metal-filesystem-pipeline/filesystem_context.hpp>

#include <dirent.h>
#include <sys/stat.h>

namespace metal {

FilesystemContext::FilesystemContext(std::string metadataDir,
                                     mtl_storage_backend *storage,
                                     bool deleteMetadataIfExists) {
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

  mtl_initialize(&_context, metadataDir.c_str(), storage);
}

}  // namespace metal
