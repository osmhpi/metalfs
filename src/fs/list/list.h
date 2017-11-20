#pragma once

#include <stdbool.h>

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
