#ifndef _V_HTTP_REQUEST_H
#define _V_HTTP_REQUEST_H

#include "vhash_table.h"
#include "vstring.h"
#include "vmime_type.h"
#include <stdbool.h>

typedef enum vhttp_method {
  VHTTP_METHOD_ERROR,
  
  VHTTP_OPTIONS,
  VHTTP_GET,
  VHTTP_HEAD,
  VHTTP_POST,
  VHTTP_PUT,
  VHTTP_PATCH,
  VHTTP_DELETE,
  VHTTP_TRACE,
  VHTTP_CONNECT,
} vhttp_method;

typedef enum vhttp_status {
    // 1xx - Informational
    VHTTP_CONTINUE = 100,
    VHTTP_SWITCHING_PROTOCOLS = 101,
    VHTTP_PROCESSING = 102, // WebDAV

    // 2xx - Success
    VHTTP_OK = 200,
    VHTTP_CREATED = 201,
    VHTTP_ACCEPTED = 202,
    VHTTP_NO_CONTENT = 204,

    // 3xx - Redirection
    VHTTP_MOVED_PERMANENTLY = 301,
    VHTTP_FOUND = 302,
    VHTTP_SEE_OTHER = 303,
    VHTTP_NOT_MODIFIED = 304,
    VHTTP_TEMPORARY_REDIRECT = 307,
    VHTTP_PERMANENT_REDIRECT = 308,

    // 4xx - Client Errors
    VHTTP_BAD_REQUEST = 400,
    VHTTP_UNAUTHORIZED = 401,
    VHTTP_FORBIDDEN = 403,
    VHTTP_NOT_FOUND = 404,
    VHTTP_METHOD_NOT_ALLOWED = 405,
    VHTTP_REQUEST_TIMEOUT = 408,
    VHTTP_CONFLICT = 409,
    VHTTP_PAYLOAD_TOO_LARGE = 413,
    VHTTP_TOO_MANY_REQUESTS = 429,

    // 5xx - Server Errors
    VHTTP_INTERNAL_SERVER_ERROR = 500,
    VHTTP_NOT_IMPLEMENTED = 501,
    VHTTP_BAD_GATEWAY = 502,
    VHTTP_SERVICE_UNAVAILABLE = 503,
    VHTTP_GATEWAY_TIMEOUT = 504

} vhttp_status;

/***********TODO***********/
/* [] Accept              */
/* [] Accept-Charset      */
/* [] Accept-Encoding     */
/* [] Accept-Language     */
/* [] Authorization       */
/* [] Expect              */
/* [] From                */
/* [] Host                */
/* [] If-Match            */
/* [] If-Modified-Since   */
/* [] If-None-Match       */
/* [] If-Range            */
/* [] If-Unmodified-Since */
/* [] Max-Forwards        */
/* [] Proxy-Authorization */
/* [] Range               */
/* [] Referer             */
/* [] TE		  */
/* [] User-Agent	  */
/**************************/

static const char* HttpHeaders[] =  {
  "allow",
  "content-encoding",
  "content-language",
  "content-length",
  "content-location",
  "content-md5",
  "content-range",
  "content-type",
  "expires",
  "last-modified",
  "extension-header",
  "accept",
  "accept-charset",
  "accept-encoding",
  "accept-language",
  "authorization",
  "cache-control",
  "connection",
  "date",
  "pragma",
  "trailer",
  "transfer-encoding",
  "upgrade",
  "via",
  "warning",
  "expect",
  "from",
  "host",
  "if-match",
  "if-modified-since",
  "if-range",
  "if-unmodified-since",
  "max-fowards",
  "proxy-authorization",
  "range",
  "referer",
  "te",
  "user-agent",
};


static const char* HttpMethodsStr[] = {
    [VHTTP_GET]    = "GET",
    [VHTTP_POST]   = "POST",
    [VHTTP_PUT]    = "PUT",
    [VHTTP_DELETE] = "DELETE",
    [VHTTP_OPTIONS] = "OPTIONS",
    [VHTTP_HEAD]   = "HEAD",
    [VHTTP_PATCH]  = "PATCH",
    [VHTTP_TRACE]  = "TRACE",
    [VHTTP_CONNECT] = "CONNECT",
};

typedef enum {
  JAPABMS_DA_SILVA,
} vhttp_error;

#define CR '\r'
#define LF '\n'
#define CLRF "\r\n\r\n"

typedef struct vhttp_request vhttp_request;
struct vhttp_request {
  vhttp_status Status;
  
  vhttp_method Method;
  vstring* StrMethod;
  
  vstring* Url;
  
  vhash_table* Headers;

  u64 Length;
  
  vstring* Body;
  u64 ContentLength;
  mime_mapping ContentType;

  enum {
    VHTTP_CONNECTION_CLOSE,
    VHTTP_CONNECTION_KEEP_ALIVE
  } Connection;

};

void VHttpRequest_Init(vhttp_request* Request);
void VHttpRequest_Destroy(vhttp_request* Request);
bool VHttpRequest_Parse(vhttp_request* Request, vstring* RawRequest);

#endif
