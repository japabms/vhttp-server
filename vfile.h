#ifndef _V_FILE_H_
#define _V_FILE_H_

#include <sys/stat.h>
#include <fcntl.h>
#include "vstring.h"

typedef struct stat vfile_info;

typedef enum vfile_type {
  VFILE_BLOCK_DEV,
  VFILE_CHAR_DEV,
  VFILE_DIRECTORY,
  VFILE_SOCKET,
  VFILE_REGULAR,
  VFILE_PIPE,
  VFILE_SIMLINK,
} vfile_type;

typedef struct vfile vfile;
struct vfile {
  s32 FileDescriptor;
  vfile_info Info;
  vstring* Path;
};

vfile VFile_Open(const char* FilePath, s32 Flags);
void VFile_Close(vfile* File);
vfile_type VFile_GetType(vfile* File);

#endif

