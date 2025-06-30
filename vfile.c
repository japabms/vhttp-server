#include "vfile.h"
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

vfile VFile_Open(const char* FilePath, s32 Flags) {
  vfile File = {0};
  vfile_info Info = {0};

  if(FilePath == NULL) {
    File.FileDescriptor = -1;
    return File;
  }
  
  if((File.FileDescriptor = open(FilePath, Flags)) < 0) {
    fprintf(stderr, "open: %s %s\n", strerror(errno), FilePath);
    return File;
  }
  
  if(fstat(File.FileDescriptor, &Info) < 0) {
    perror("ftstat");
    return File;
  }
  
  File.Info = Info;

  return File;
}

vfile_type VFile_GetType(vfile* File) {
  switch (File->Info.st_mode & S_IFMT) {
    case S_IFBLK: return VFILE_BLOCK_DEV;
    case S_IFCHR: return VFILE_CHAR_DEV;
    case S_IFDIR: return VFILE_DIRECTORY;
    case S_IFIFO: return VFILE_PIPE;
    case S_IFLNK: return VFILE_SIMLINK;
    case S_IFREG: return VFILE_REGULAR;
    case S_IFSOCK: return VFILE_SOCKET;
  }

  return -1;
}

void VFile_Close(vfile* File) {
  close(File->FileDescriptor);
  File->FileDescriptor = -1;
}

