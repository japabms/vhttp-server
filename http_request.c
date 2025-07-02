#include "http_request.h"
#include "vutil.h"
#include "vstring.h"
#include <ctype.h>
#include <string.h>

#define VHTTP_HEADER_KEY_MAX 512
#define VHTTP_HEADER_VALUE_MAX 1024

static vhttp_method VHttpRequest_GetMethodFromString(const vstring* Method) {
  if(Method->Length > 7) {
    return VHTTP_METHOD_ERROR;
  }
  
  if(memcmp(Method->Data, "GET", 3) == 0) {
    return VHTTP_GET;
  } else if(memcmp(Method->Data, "HEAD", 4) == 0) {
    return VHTTP_HEAD;
  } else if (memcmp(Method->Data, "POST", 4) == 0) {
    return VHTTP_POST;
  } else if(memcmp(Method->Data, "PUT", 3) == 0) {
    return VHTTP_PUT;
  } else if(memcmp(Method->Data, "DELETE", 6) == 0) {
    return VHTTP_DELETE;
  } else if(memcmp(Method->Data, "TRACE", 5) == 0) {
    return VHTTP_TRACE; 
  } else if(memcmp(Method->Data, "CONNECT", 7) == 0) {
    return VHTTP_CONNECT; 
  } else if(memcmp(Method->Data, "PATCH", 5) == 0) {
    return VHTTP_PATCH;
  }
  return VHTTP_METHOD_ERROR;
}

static void VHttpRequest_DecodeUrl(vhttp_request* Request, const vstring_view* Url) {
  u64 Count = 0;
  for(int Idx = 0; Idx < Url->Length; Idx++) {
    switch(Url->Data[Idx]) {
      case '%': {
        // TODO: check for more than 2 hex
        if(isxdigit(Url->Data[Idx+1]) && isxdigit(Url->Data[Idx+2])) {
          char Hex[3] = {Url->Data[Idx+1], Url->Data[Idx+2], '\0'};
          VString_Insert(Request->Url, Count++, (char) strtol(Hex, NULL, 16));
          Idx += 2;
        } else {
          VString_Insert(Request->Url, Count++, Url->Data[Idx]);
        }
        break;        
      }
      case '+': {
        VString_Insert(Request->Url, Count++, ' ');
      }
      default: {
        VString_Insert(Request->Url, Count++, Url->Data[Idx]);
        break;        
      }
    }
  }
}

// Ad hoc parser
bool VHttpRequest_Parse(vhttp_request* Request, vstring* RawRequest) {
  vstring_view Split[2];
  VString_SplitBySubstring(RawRequest, "\r\n\r\n", Split, 2);

  vstring_view* RequestHead = &Split[0];
  Request->Length = RequestHead->Length + 4;

  vstring_view RequestLines[64] = {0};
  u32 SplitCount = VString_SplitBySubstring((vstring*)RequestHead, "\r\n", RequestLines, 64);
  for(s32 I = 1; I < SplitCount; I++) {
    
    int Idx = VString_FindFirstCharOccurence((vstring*)&RequestLines[I], ':');

    // FIXME: stdup this strings..
    vstring_view Key = VStringView_FromRange((vstring*)&RequestLines[I], 0, Idx);
    vstring_view Value = VStringView_FromRange((vstring*)&RequestLines[I], Idx+1, RequestLines[I].Length - Idx);

    if(Key.Length == 0|| Value.Length == 0) return false;
    
    char CKey[Key.Length];
    memcpy(CKey, Key.Data, Key.Length);
    CKey[Key.Length] = '\0';
    // Check if key or value length is > 0
    
    // VString_ToLower((vstring*)&KeyValuePair[0]);
    VString_Trim((vstring*)&Value);
    Value.Data[Value.Length] = '\0';
    
    // FIXME: leaking here.
    VHashTable_Insert(Request->Headers, (uchar*) CKey, Value.Data, Value.Length + 1);


    /**************************************************************************************/
    /* switch(tolower(Key.Data[0])) {                                                     */
    /*   case 'a':                                                                        */
    /*     {                                                                              */
    /*       if(memcmp(Key.Data, "Accept", Key.Length) == 0) {                            */
    /*         // printf("Accept header\n");                                              */
    /*       }                                                                            */
    /*     } break;                                                                       */
    /*   case 'c':                                                                        */
    /*     {                                                                              */
    /*                                                                                    */
    /*       if(memcmp(Key.Data, "Connection", Key.Length) == 0) {                        */
    /*         // printf("Connection header\n");                                          */
    /*       } else if(memcmp(Key.Data, "Content-Length", Key.Length) == 0) {             */
    /*         Request->ContentLength = atoi(Value.Data); // TODO: change this atoi call. */
    /*         // printf("Request->ContentLength = %ld\n", Request->ContentLength);       */
    /*       } else if(memcmp(Key.Data, "Content-Type", Key.Length) == 0) {               */
    /*                                                                                    */
    /*       } else if(memcmp(Key.Data, "Content-Encoding", Key.Length) == 0) {           */
    /*                                                                                    */
    /*       } else if(memcmp(Key.Data, "Content-Language", Key.Length) == 0) {           */
    /*                                                                                    */
    /*       } else if(memcmp(Key.Data, "Content-Location", Key.Length) == 0) {           */
    /*                                                                                    */
    /*       } else if(memcmp(Key.Data, "Content-Range", Key.Length) == 0) {              */
    /*                                                                                    */
    /*       } else if(memcmp(Key.Data, "Cache-Control", Key.Length) == 0) {              */
    /*                                                                                    */
    /*       }                                                                            */
    /*     } break;                                                                       */
    /*   case 'e':                                                                        */
    /*     {                                                                              */
    /*       if(memcmp(Key.Data, "Expect", Key.Length) == 0) {                            */
    /*                                                                                    */
    /*       }                                                                            */
    /*     } break;                                                                       */
    /*   default:                                                                         */
    /*     {                                                                              */
    /*       //printf("%s\n", Key->Data);                                                 */
    /*     } break;                                                                       */
    /* }                                                                                  */
    /**************************************************************************************/
  }

  vstring_view Method_Url_Version[3];
  SplitCount = VString_SplitBySubstring((vstring*)&RequestLines[0], " ", Method_Url_Version, 3);
  if(SplitCount > 3) {
    printf("requestline error\n");
    return false;
  }
  
  vstring_view* Method = &Method_Url_Version[0];
  vstring_view* Url = &Method_Url_Version[1];
  vstring_view* Version = &Method_Url_Version[2];
  
  // Checking if the method is valid.
  Request->Method = VHttpRequest_GetMethodFromString((vstring*) Method);
  if(Request->Method == VHTTP_METHOD_ERROR) {
    return false;
  }
  VString_CopyCString(Request->StrMethod, HttpMethodsStr[Request->Method]);

  // Decoding percent url
  VHttpRequest_DecodeUrl(Request, Url);
    
  // Check if the HTTP version is correct
  if(strncmp(Version->Data, "HTTP/1.1", Version->Length) != 0) {
    return false;
  }
  
  return true;
}

// Remember to free this shit
void VHttpRequest_Init(vhttp_request* Request) {
  Request->Method = VHTTP_METHOD_ERROR;
  Request->Url = VString_Init(NULL);
  Request->StrMethod = VString_Init(NULL);
  Request->Body = VString_Init(NULL);
  Request->Headers = VHashTable_Init(0);

  Request->ContentLength = Request->Length = 0;
}

void VHttpRequest_Destroy(vhttp_request* Request) {
  VString_Free(Request->Body);
  VString_Free(Request->Url);
  VString_Free(Request->StrMethod);
  VHashTable_Free(Request->Headers);
}
