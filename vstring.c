#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "vstring.h"
#include "vfile.h"

#define INIT_TEXT_SIZE 512

#define VStringView_Static(text) (vstring_view) { text, sizeof(text) - 1}

vstring* VString_InitWithCapacity(char* Text, u64 Capacity) {
  vstring* String = calloc(1, sizeof(vstring));
  assert(String != NULL);

  // Check for capacity

  if(Capacity == 0) {
    String->Capacity = INIT_TEXT_SIZE;    
  } else {
    String->Capacity = Capacity;    
  }

  // Alloc capacity
  String->Data = (char*) calloc(1, String->Capacity);
  
  if(Text != NULL) {
    VString_CopyCString(String, Text);
  } else {
    String->Length = 0;	
  }

  return String;
}

void VString_Resize(vstring* String, u64 Capacity) {
  if(Capacity == 0) {
    String->Capacity = String->Capacity == 0 ? INIT_TEXT_SIZE : String->Capacity * 2;    
  } else {
    String->Capacity = Capacity;
  }

  String->Data = (char*) realloc(String->Data, String->Capacity * sizeof(char));
  assert(String->Data != NULL);
}

#define VString_Realloc(String) VString_Resize(String, 0)

void VString_Append(vstring* String, char Ch) {
  assert(String != NULL && String->Data != NULL);
  
  if(String->Length >= String->Capacity) {
    VString_Realloc(String);
  }

  String->Data[String->Length++] = Ch;
}

void VString_Insert(vstring* String, u64 Idx, char Ch) {
  assert(String != NULL && String->Data != NULL);

  if(String->Length + 1 >= String->Capacity) {
    VString_Realloc(String);
  }
  memmove(&String->Data[Idx+1], &String->Data[Idx], String->Length - Idx);

  String->Data[Idx] = Ch;
  String->Length++;
}

void VString_Remove(vstring *String, u64 Idx) {
  assert(String != NULL && String->Data != NULL);
  if(Idx >= String->Length) return;

  if(Idx < String->Length) {
    memmove(&String->Data[Idx], &String->Data[Idx+1], String->Length - Idx - 1);
  }
  
  String->Length--;
}

void VString_Concat(vstring* Dest, vstring* Src) {
  while(Src->Length + Dest->Length >= Dest->Capacity) {
    VString_Realloc(Dest);
  }
  
  for(s32 Idx = 0; Idx < Src->Length; Idx++) {
    VString_Append(Dest, Src->Data[Idx]);
  }
}

void VString_ConcatCString(vstring* Dest, char* Src) {
  s64 SrcLength = strlen(Src);
  while(SrcLength + Dest->Length >= Dest->Capacity) {
    VString_Realloc(Dest);
  }

  for(s32 Idx = 0; Idx < SrcLength; Idx++) {
    VString_Append(Dest, Src[Idx]);
  }
}

u64 VString_Copy(vstring* Dest, vstring* Src) {
  if(Dest == NULL || Dest->Data == NULL || Src == NULL || Src->Data == NULL)
    return 0;

  while(Src->Length  >= Dest->Capacity) {
    VString_Realloc(Dest);
  }

  if(Dest->Length != 0) memset(Dest->Data, 0, Dest->Length);

  for(u64 i = 0; i < Src->Length; i++) {
    Dest->Data[i] = Src->Data[i];
  }
  
  Dest->Length = Src->Length;
  
  return Dest->Length;
}

u64 VString_CopyCString(vstring* Dest, const char* Src) {
  if(Dest == NULL || Dest->Data == NULL) {
    return 0;
  }
  
  u64 SrcLen = strlen(Src);
  while(SrcLen >= Dest->Capacity) {
    VString_Realloc(Dest);
  }

  if(Dest->Length != 0) memset(Dest->Data, 0, Dest->Length);
		
  for(u64 i = 0; i < SrcLen; i++) {
    VString_Append(Dest, Src[i]);
  }

  Dest->Length = SrcLen;

  return SrcLen;
}


/**
 * Read entire file contents and stores into a vstring
 * @param String The vstring that will recieve the data.
 * @param Path Path to the file that will be readed.
 * @return Returns a number representing how many data was readed.
 */
u64 VString_ReadEntireFile(vstring* String, const char* Path) {
  u64 Read = 0;
  vfile File;

  if((File = VFile_Open(Path, O_RDONLY)).FileDescriptor < 0) {
    return Read;
  }

  if((File.Info.st_mode & S_IFMT) != S_IFREG) {
    fprintf(stderr, "[VString] ReadEntirefile: The file is not a regular file\n");
    VFile_Close(&File);
    return Read;
  }
 
  if(File.Info.st_size >= String->Capacity) {
    VString_Resize(String, File.Info.st_size);
  }
  
  Read = read(File.FileDescriptor, String->Data, File.Info.st_size);
  
  if(Read != File.Info.st_size) {
    fprintf(stderr, "[VString] ReadEntirefile: Cannot read entire file\n");
    String->Length = Read;
    return Read;
  }

  String->Length = File.Info.st_size;
  VFile_Close(&File);
  
  return Read;
}

// This only works with ASCII
void VString_ToLower(vstring* String) {
  for(s32 Idx = 0; Idx < String->Length; Idx++) {
    if(isalpha(String->Data[Idx])) {
      String->Data[Idx] = tolower(String->Data[Idx]);
    }
  }
}

void VString_Trim(vstring* String) {
  s32 Start = 0, End = String->Length - 1;
  while(isspace(String->Data[Start])) {
    Start++;
  }

  while(isspace(String->Data[End])) {
    End--;
  }

  s32 NewLen = End - Start + 1;
  
  memmove(String->Data, String->Data + Start, NewLen);

  String->Length = NewLen;
}

s64 VStringView_ToCString(const vstring_view* View, char* Out, u32 OutSize) {
  s64 Size = OutSize < View->Length ? OutSize : View->Length;
  memcpy(Out, View->Data, Size);
  // TODO: add null terminator if View->Length > OutSize.
  return Size;
}

s64 VString_FindLastCharOccurence(const vstring* String, s64 Ch) {
  for(s32 Idx = String->Length - 1; Idx > 0; Idx--) {
    if(String->Data[Idx] == Ch) {
      return Idx;
    }
  }

  return -1;
}

s64 VString_FindFirstCharOccurence(const vstring* String, s64 Ch) {
  for(s32 Idx = 0; Idx < String->Length; Idx++) {
    if(String->Data[Idx] == Ch) {
      return Idx;
    }
  }

  return -1;
}

void VString_Println(const vstring* String) {
  printf("%.*s\n", (int) String->Length, String->Data);
}

void VStringView_Println(const vstring_view* View) {
  printf("%.*s\n", (int) View->Length, View->Data);
}

vstring_view VStringView_FromRange(const vstring* String, u64 Start, u64 End) {
  vstring_view Result;
  Result.Data = String->Data + Start;
  Result.Length = End;

  return Result;
}

vstring_view VStringView_FromCString(const char* C_String) {
  vstring_view Result;
  
  Result.Data = C_String;
  Result.Length = strlen(C_String);

  return Result;  
}

/**
 * Split string
 * @param String The vstring that will be splited.
 * @param Substring The substring that.
 * @param[out] Result An array of string_view that the function will populate with.
 * @param Size Result size.
 * @return Returns a count of how many splits the function added to Result array
 */
u64 VString_SplitBySubstring(const vstring* String, const char* Substring, vstring_view* Result, u32 Size) {
  if(!String || !Substring || !Result) {
    return 0;
  }

  vstring_view View = VStringView_FromCString(Substring); // i probably dont need this
  if(View.Length <= 0) {
    return 0;
  }
  
  vstring_view Split;
  u32 ResultCursor = 0;
  u32 Check = 0;
  u32 Start = 0;

  for(s64 I = 0; I < String->Length; I++) {
    if(String->Data[I] == View.Data[Check]) {
      Check += 1;
      if(View.Length == Check) {
        if(ResultCursor < Size) {
          u64 Len = (I + 1) - View.Length - Start;
          Split = VStringView_FromRange(String, Start, Len);
          if(Split.Length != 0) {
            Result[ResultCursor++] = Split;	    
          }
          Start = I + 1;
          Check = 0;
        } else {
          return ResultCursor;
        }
      }
    } else {
      Check = 0;
    }
  }

  Split = VStringView_FromRange(String, Start, String->Length - Start);
  Result[ResultCursor++] = Split;

  return ResultCursor;
}

u64 VString_FindSubstring(const vstring* String, const char* Substring) {
  if(String == NULL || Substring == NULL) return 0;
  
  s32 SubstringLen = strlen(Substring);
  
  if(SubstringLen == 0) return 0;
  
  //  char* Ptr = memmem(String->Data, String->Length, Substring, SubstringLen);

  //  s64 Idx2 = Ptr - String->Data;
  
  //  printf("%ld\n", Idx2);
  
  for(int Idx = 0; Idx < String->Length; Idx++) {
    if(memcmp(&String->Data[Idx], Substring, SubstringLen) == 0) {
      return Idx + SubstringLen;
    }
  }
  
  return 0;
}

void VString_Free(vstring* String) {
  free(String->Data);
  free(String);
}
