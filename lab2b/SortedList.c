// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495
// SLIPDAYS: 3

#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#include "SortedList.h"

void insert_helper(SortedListElement_t *current, SortedListElement_t *element) {
  current->prev->next = element;
  element->prev = current->prev;
  element->next = current;
  current->prev = element;
}

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  if (list == NULL || element == NULL) return;

  SortedListElement_t *current = list->next;
  while (current != list) {
    if (strcmp(current->key, element->key) <= 0)
      break;
    current = current->next;
  }

  // Critical section
  if (opt_yield & INSERT_YIELD)
    sched_yield();

  insert_helper(current, element);
}


int SortedList_delete(SortedListElement_t *element) {
  if (element->next->prev != element ||
      element->prev->next != element)
    return 1;

  // Critical section
  if (opt_yield & DELETE_YIELD)
    sched_yield();
  
  element->prev->next = element->next;
  element->next->prev = element->prev;
  //temp->next = element->next;
  //element->next->prev = temp;

  return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if (list == NULL)
    return NULL;

  SortedListElement_t *current = list->next;
  while (current != list) {
    if (strcmp(current->key, key) == 0) 
      return current;

    // Critical section
    if (opt_yield & LOOKUP_YIELD)
      sched_yield();

    current = current->next;
  }

  return NULL;
}

int SortedList_length(SortedList_t *list) {
  if (list == NULL) 
    return -1;

  int counter = 0;
  SortedListElement_t *current = list->next;
  while (current != list) {
    if (current->prev->next != current ||
	current->next->prev != current)
      return -1;

    // Critical section
    if (opt_yield & LOOKUP_YIELD) 
      sched_yield();

    ++counter;
    current = current->next;
  }

  return counter;
}
