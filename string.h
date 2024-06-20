#pragma once

#include "list.h"
#include <stdbool.h>

DECLARE_LIST(ListChar, char);

typedef struct String 
{
	ListChar* list; 
} String;

DECLARE_LIST(ListString, String*);

String* String_New();
void String_AppendChar(String* str, char c);
void String_RemoveAt(String* str, size_t index);
size_t String_GetLength(String* str);
char* String_GetCString(String* str);
void String_AppendData(String* str, char* data, size_t numChars);
void String_AppendString(String* str, String* other);
void String_AppendCString(String* str, char* cstr);
char String_GetCharAt(String* str, size_t index);
ListString* String_Split(String* str, char delimiter);
bool String_EqualsCString(String* str, char* cstr);
bool String_EqualsString(String* str, String* other);
void String_Destroy(String* str);
String* String_Join(ListString* strList, char delimiter);
void String_Reset(String* str);
String* String_Copy(String* str);
String* String_Itoa(int x);
bool String_Atoi(String* str, int* outInt);

size_t GetCStringLength(char* cstr);