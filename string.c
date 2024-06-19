#include "string.h"
#include <stdio.h>

DEFINE_LIST(ListChar, char);
DEFINE_LIST(ListString, String*);

String* String_New()
{
	String* str = (String*)malloc(sizeof(String));
	assert(str);
	str->list = NULL;
	String_Reset(str);
	return str;
}

void String_AppendChar(String* str, char c)
{
	assert(str);
	ListChar_Remove(str->list, str->list->numElements - 1);
	ListChar_Add(str->list, c);
	ListChar_Add(str->list, '\0');
}

void String_RemoveAt(String* str, size_t index)
{
	assert(str);
	ListChar_Remove(str->list, index);
}

size_t String_GetLength(String* str)
{
	assert(str);
	return str->list->numElements - 1;
}

char* String_GetCString(String* str)
{
	assert(str);
	assert(ListChar_Get(str->list, str->list->numElements - 1) == '\0');
	return str->list->data;
}

void String_AppendData(String* str, char* data, size_t numChars)
{
	assert(str);
	assert(data);
	ListChar_Remove(str->list, str->list->numElements - 1);
	ListChar_Append(str->list, data, numChars);
	ListChar_Add(str->list, '\0');
}

void String_AppendString(String* str, String* other)
{
	assert(str);
	assert(other);
	String_AppendData(str, other->list->data, other->list->numElements);
}

void String_AppendCString(String* str, char* cstr)
{
	assert(str);
	assert(cstr);
	size_t cstrLength = GetCStringLength(cstr);
	String_AppendData(str, cstr, cstrLength);
}

char String_GetCharAt(String* str, size_t index)
{
	assert(index >= 0 && index < String_GetLength(str));
	return ListChar_Get(str->list, index);
}

ListString* String_Split(String* str, char delimiter)
{
	ListString* listStr = ListString_New();
	String* currentStr = String_New();

	for (size_t i = 0; i < String_GetLength(str); i++)
	{
		if (String_GetCharAt(str, i) == delimiter)
		{
			ListString_Add(listStr, currentStr);
			currentStr = String_New();
			continue;
		}

		String_AppendChar(currentStr, String_GetCharAt(str, i));
	}

	ListString_Add(listStr, currentStr);
	return listStr;
}

bool String_EqualsCString(String* str, char* cstr)
{
	size_t cstrLength = GetCStringLength(cstr);
	if (String_GetLength(str) !=  cstrLength)
		return false;

	for (size_t i = 0; i < cstrLength; i++)
	{
		if (String_GetCharAt(str, i) != cstr[i])
			return false;
	}

	return true;
}

bool String_EqualsString(String* str, String* other)
{
	if (String_GetLength(str) != String_GetLength(other))
		return false;

	for (size_t i = 0; i < String_GetLength(str); i++)
	{
		if (String_GetCharAt(str, i) != String_GetCharAt(other, i))
			return false;
	}

	return true;
}

void String_Destroy(String* str)
{
	assert(str);
	ListChar_Destroy(str->list);
	free(str);
}

String* String_Join(ListString* strList, char delimiter)
{
	assert(strList);
	String* str = String_New();

	for(size_t i = 0; i < strList->numElements; i++)
	{
		String_AppendString(str, ListString_Get(strList, i));
		String_AppendChar(str, delimiter);
	}

	String_RemoveAt(str, String_GetLength(str) - 1);
	return str;
}

void String_Reset(String* str)
{
	if (str->list)
		ListChar_Destroy(str->list);

	str->list = ListChar_New();
	ListChar_Add(str->list, '\0');
}

String* String_Itoa(int x)
{
	String* str = String_New();
	size_t len = snprintf(NULL, 0, "%d", x);
	char* cstr = (char*)malloc(sizeof(char) * len + 1);
	snprintf(cstr, 0, "%d", x);
	String_AppendCString(str, cstr);
	free(cstr);
	return str;
}

size_t GetCStringLength(char* cstr)
{
	size_t cstrLength = 0;
	while(1)
	{
		if (cstr[cstrLength] == '\0')
			break;

		cstrLength++;
	}

	return cstrLength;
}