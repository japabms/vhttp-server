#include "vmime_type.h"
#include <string.h>

mime_mapping GetMimeType(const char *Extension) {
  if(Extension != NULL) {
    for(int Idx = 0; G_MimeTable[Idx].Extension != NULL; Idx++) {
      if (strcmp(Extension, G_MimeTable[Idx].Extension) == 0) {
        return G_MimeTable[Idx];
      }
    }    
  }

  return G_MimeTable[sizeof(G_MimeTable)/sizeof(*G_MimeTable) - 1];
}

