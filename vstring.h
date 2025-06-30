#ifndef _V_STRING_
#define _V_STRING_

#include "vutil.h"
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64)
#define VSTRING_EXPORT __declspec(dllexport)
#elif defined(linux) || defined(__linux__)
#define VSTRING_EXPORT __attribute__((visibility("default")))
#else
#define VSTRING_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// String Builder
typedef struct {
  char* Data;
  u64 Length;
  u64 Capacity;

} vstring;

typedef struct {
  char* Data;
  u64 Length;
} vstring_view;

VSTRING_EXPORT vstring* VString_InitWithCapacity(char* Text, u64 Capacity);
#define VString_Init(Text) VString_InitWithCapacity((Text), 0)
VSTRING_EXPORT void VString_Append(vstring* String, char Ch);
VSTRING_EXPORT void VString_Insert(vstring* String, u64 Idx, char Ch);
VSTRING_EXPORT void VString_Remove(vstring *String, u64 Idx);
VSTRING_EXPORT u64 VString_Copy(vstring* Dest, vstring* Src);
VSTRING_EXPORT u64 VString_CopyCString(vstring* Dest, const char* Src);
VSTRING_EXPORT u64 VString_ReadEntireFile(vstring *String, const char *path);
VSTRING_EXPORT void VString_Println(const vstring *String);
VSTRING_EXPORT void VString_Concat(vstring* Dest, vstring* Src);
VSTRING_EXPORT void VString_ConcatCString(vstring* Dest, char* Src);
void VString_Resize(vstring* String, u64 Capacity);
void VString_Free(vstring* String);
// void VString_GetLines(vstring* String, vstring_view* LinesOut, u64 Size);
vstring_view VStringView_FromRange(const vstring* String, u64 Start, u64 End);
vstring_view VStringView_FromCString(const char* C_String);
u64 VString_SplitBySubstring(const vstring* String, const char* Substring, vstring_view* Result, u32 Size);
u64 VString_FindSubstring(const vstring* String, const char* Substring);
s64 VStringView_ToCString(const vstring_view* View, char* Out, u32 OutSize);
void VString_ToLower(vstring* String);
void VString_Trim(vstring* String);
s64 VString_FindFirstCharOccurence(const vstring* String, s64 Ch);
s64 VString_FindLastCharOccurence(const vstring* String, s64 Ch);
  
#define SText(String) (String)->Data
#define SLast(String) (String)->Data[(String)->Length]
#define SFirst(String) (String)->Data[0]
#define SLength(String) (String)->Length

#ifdef __cplusplus
}
#endif

#endif
