#pragma once
#ifdef __APPLE__
#include <unistd.h>
#include <sys/stat.h>

#define _access access
#define _S_IREAD S_IRUSR
#define _S_IWRITE S_IWUSR

inline int _filelength(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) return (int)st.st_size;
    return -1;
}

#endif
