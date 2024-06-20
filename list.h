#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define INVALID_INDEX -1

#define DECLARE_LIST(ListName, ElementType) \
typedef struct ListName \
{ \
	ElementType* data; \
	size_t capacity; \
	size_t numElements; \
} ListName; \
\
ListName* ListName##_New(); \
void ListName##_ReserveCapacity(ListName* list, size_t newCapacity); \
void ListName##_Add(ListName* list, ElementType element); \
ElementType ListName##_Remove(ListName* list, size_t index); \
void ListName##_Clear(ListName* list); \
void ListName##_Insert(ListName* list, ElementType element, size_t index); \
void ListName##_Append(ListName* list, ElementType* array, size_t numElements); \
ElementType ListName##_Get(ListName* list, size_t index); \
size_t ListName##_Find(ListName* list, ElementType element); \
ListName* ListName##_Copy(ListName* list); \
void ListName##_Destroy(ListName* list);

#define DEFINE_LIST(ListName, ElementType) \
ListName* ListName##_New() \
{ \
	ListName* list = (ListName*)malloc(sizeof(ListName)); \
	assert(list); \
	 \
	list->data = NULL; \
	list->capacity = 0; \
	list->numElements = 0; \
	ListName##_ReserveCapacity(list, INITIAL_CAPACITY); \
 \
	return list; \
} \
\
void ListName##_ReserveCapacity(ListName* list, size_t newCapacity) \
{ \
	assert(list); \
	assert(newCapacity > 0); \
	if (newCapacity <= list->capacity) \
		return; \
\
	ElementType* newData = (ElementType*)calloc(newCapacity, sizeof(ElementType)); \
	assert(newData); \
\
	if (list->data) \
	{ \
		memcpy(newData, list->data, list->numElements * sizeof(ElementType)); \
		free(list->data); \
	} \
\
	list->data = newData; \
	list->capacity = newCapacity; \
} \
\
void ListName##_Add(ListName* list, ElementType element) \
{ \
	assert(list); \
	while (list->numElements + 1 > list->capacity) \
		ListName##_ReserveCapacity(list, list->capacity * 2); \
\
	list->data[list->numElements] = element; \
	list->numElements++; \
} \
\
ElementType ListName##_Remove(ListName* list, size_t index) \
{ \
	assert(list); \
	assert(index >= 0 && index < list->numElements); \
	ElementType element = list->data[index]; \
	memmove(list->data + index, list->data + index + 1 , (list->numElements - index - 1) * sizeof(ElementType)); \
	memset(list->data + (list->numElements - 1), 0, sizeof(ElementType)); \
	list->numElements--; \
	return element; \
} \
\
void ListName##_Clear(ListName* list) \
{ \
	assert(list); \
	free(list->data); \
	list->data = NULL; \
	list->capacity = 0; \
	list->numElements = 0; \
	ListName##_ReserveCapacity(list, INITIAL_CAPACITY); \
} \
\
void ListName##_Insert(ListName* list, ElementType element, size_t index) \
{ \
	assert(list); \
	assert(index >= 0 && index <= list->numElements); \
	while (list->numElements + 1 > list->capacity) \
		ListName##_ReserveCapacity(list, list->capacity * 2); \
\
	memmove(list->data + index + 1, list->data + index, (list->numElements - index) * sizeof(ElementType)); \
	list->data[index] = element; \
	list->numElements++; \
} \
\
void ListName##_Append(ListName* list, ElementType* array, size_t numElements) \
{ \
	assert(list); \
	while (list->numElements + numElements > list->capacity) \
		ListName##_ReserveCapacity(list, list->capacity * 2); \
\
	memcpy(list->data + list->numElements, array, numElements * sizeof(ElementType)); \
	list->numElements += numElements; \
} \
\
ElementType ListName##_Get(ListName* list, size_t index) \
{ \
	assert(list); \
	assert(index >= 0 && index < list->numElements); \
	return list->data[index]; \
} \
\
size_t ListName##_Find(ListName* list, ElementType element) \
{ \
	assert(list); \
	assert(element); \
	for (size_t i = 0; i < list->numElements; i++) \
	{ \
		if (ListName##_Get(list, i) == element) \
			return i; \
	} \
\
	return INVALID_INDEX; \
} \
\
ListName* ListName##_Copy(ListName* list) \
{ \
	assert(list); \
	ListName* copy = ListName##_New(); \
	ListName##_ReserveCapacity(copy, list->capacity); \
	memcpy(copy->data, list->data, list->numElements * sizeof(ElementType)); \
	copy->numElements = list->numElements; \ 
} \
\
void ListName##_Destroy(ListName* list) \
{ \
	assert(list); \
	free(list->data); \
	free(list); \
}
