#ifndef _V_MIME_TYPE_H_
#define _V_MIME_TYPE_H_

#include <stdlib.h>
#include "vutil.h"

typedef struct {
  const char *Extension;
  const char *MimeType;
  int UseSendfile;
} mime_mapping;

// TODO: use a hashtable for better performance.
static mime_mapping G_MimeTable[] = {
  {".html", "text/html; charset=utf-8", 0},
  {".htm", "text/html; charset=utf-8", 0},
  {".css", "text/css", 0},
  {".js", "text/javascript", 0},
  {".json", "application/json", 0},
  {".xml", "application/xml", 0},
  {".txt", "text/plain; charset=utf-8", 0},
  {".pdf", "application/pdf", 1},
  {".svg", "image/svg+xml", 1},
  {".png", "image/png", 1},
  {".jpg", "image/jpeg", 1},
  {".jpeg", "image/jpeg", 1},
  {".gif", "image/gif", 1},
  {".webp", "image/webp", 1},
  {".ico", "image/x-icon", 1},
  {".mp4", "video/mp4", 1},
  {".mp3", "audio/mpeg", 1},
  {".wav", "audio/wav", 1},
  {".woff", "font/woff", 1},
  {".woff2", "font/woff2", 1},
  {".ttf", "font/ttf", 1},
  {".otf", "font/otf", 1},
  {".zip", "application/zip", 1},
  {".tar", "application/x-tar", 1},
  {".gz", "application/gzip", 1},
  {NULL, "text/plain", 0} // fallback
};

mime_mapping GetMimeType(const char *Extension);



#endif
