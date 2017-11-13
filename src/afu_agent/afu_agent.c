#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <mntent.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#define BLOCK_SIZE 512
#define FILENAME_SIZE 512

int get_process_connected_to_stdin(char * buffer, size_t bufsiz) {

    // Determine the inode number of the stdin file descriptor
    struct stat stdin_stats;
    fstat(0, &stdin_stats);
    // TODO: Check errno

    // Enumerate processes
    char stdout_filename[FILENAME_SIZE];
    char stdout_pipename[FILENAME_SIZE];
    char attached_process[FILENAME_SIZE];
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            bool isProcessId = true;
            for (int i = 0; i < strlen(dir->d_name); ++i) {
                if (!isdigit(dir->d_name[i])) {
                    isProcessId = false;
                    break;
                }
            }

            if (!isProcessId) {
                continue;
            }

            snprintf(stdout_filename, FILENAME_SIZE, "/proc/%s/fd/1", dir->d_name);

            // Check if stdout of this process is the same as our stdin pipe
            size_t len = readlink(stdout_filename, stdout_pipename, FILENAME_SIZE);
            if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 && len > sizeof("pipe:[]")) {
                int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
                if (inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_SIZE, "/proc/%s/exe", dir->d_name);
                    readlink(attached_process, buffer, bufsiz);
                    // TODO: ...
                    break;
                }
            }
        }
        closedir(d);
    } else {
        // TODO...
    }

    return 0;
}

int get_mount_point_of_filesystem(char *path, char * result, size_t size) {
    struct mntent *ent;
    FILE *mountsFile;

    mountsFile = setmntent("/proc/mounts", "r");
    if (mountsFile == NULL) {
        perror("setmntent");
        return -1;
    }
    size_t previousLongestPath = 0;
    size_t pathlen = strlen(path);
    while (NULL != (ent = getmntent(mountsFile))) {
        size_t mntlen = strlen(ent->mnt_dir);
        if (mntlen > pathlen) {
            continue;
        }

        if (mntlen < previousLongestPath) {
            continue;
        }

        if (strncmp(path, ent->mnt_dir, mntlen) == 0) {
            strncpy(result, ent->mnt_dir, mntlen+1 > size ? size : mntlen+1);
            previousLongestPath = mntlen;
        }
    }

    endmntent(mountsFile);

    if (previousLongestPath == 0) {
        return -1;
    }

    return 0;
}

int main() {

    char test[FILENAME_SIZE] = { 0 };
    get_process_connected_to_stdin(test, BLOCK_SIZE);
    printf("Connected process executable: %s\n", test);

    char ownFilename[FILENAME_SIZE];
    readlink("/proc/self/exe", ownFilename, FILENAME_SIZE);
    // TODO: ...

    char connectedExecutableFsMountPoint[FILENAME_SIZE];
    if (get_mount_point_of_filesystem(test, connectedExecutableFsMountPoint, FILENAME_SIZE))
        printf("whooopsie\n");
    char ownExecutableFsMountPoint[FILENAME_SIZE];
    if (get_mount_point_of_filesystem(ownFilename, ownExecutableFsMountPoint, FILENAME_SIZE))
        printf("oopsie\n");

    if (strcmp(connectedExecutableFsMountPoint, ownExecutableFsMountPoint) != 0) {
        printf("Processes live in different file systems!\n%s\n%s\n",
            ownExecutableFsMountPoint, connectedExecutableFsMountPoint);
    } else {
        printf("Both processes live under fs %s!\n", ownExecutableFsMountPoint);
    }

    // char buffer[BLOCK_SIZE];

    // for(;;) {
    //     size_t bytes = fread(buffer,  sizeof(char), BLOCK_SIZE, stdin);
    //     fwrite(buffer, sizeof(char), bytes, stdout);
    //     fflush(stdout);
    //     if (bytes < BLOCK_SIZE)
    //         if (feof(stdin))
    //             break;
    // }

    return 0;
}
