#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CONTAINING_LIST_RECORD(address, type) \
    ((type *)( \
    (char*)(address) - \
    (uint64_t)(&((type *)0)->Link)))

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY  *Flink;
  struct _LIST_ENTRY  *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

void InitializeListHead(PLIST_ENTRY ListHead);

bool IsListEmpty(PLIST_ENTRY ListHead);

bool RemoveEntryList(PLIST_ENTRY Entry);

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead);
PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead);

void InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
void InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
