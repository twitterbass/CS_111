// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495
// SLIPDAYS: 3

#include "SortedList.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


struct ThreadInfo {
  int index;
};

#define MUTEX 1
#define SPIN_LOCK 2
int lock_type = 0;

int numThreads    = 1;
int numIterations = 1;
int numLists      = 1;

int opt_yield  = 0;
int opt_sync   = 0;
int key_length = 3;

int *s_locks;

pthread_mutex_t *m_locks;

SortedList_t *lists;
SortedListElement_t *elements;
pthread_t *tids;
struct ThreadInfo *info;

char tag[100] = "list";

long long total_wait_time = 0;
struct timespec start, end;

static inline long long get_nanosec_from_timespec(struct timespec* spec) {
  long long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

long long time_diff(struct timespec* end, struct timespec* start) {
  return (get_nanosec_from_timespec(end) - get_nanosec_from_timespec(start));
}

void invalid_argument() {
  fprintf(stderr, "Unrecognized argument detected\n");
  exit(1);
}

void pthread_create_handler(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg) {
  if (pthread_create(thread, attr, start_routine, arg) != 0) {
    fprintf(stderr, "pthread_create() failed - %s\n", strerror(errno));
    exit(1);
  }
}

void pthread_join_handler(pthread_t thread, void **retval) {
  if (pthread_join(thread, retval) != 0) {
    fprintf(stderr, "pthread_join() failed -%s\n", strerror(errno));
    exit(1);
  }
}


void pthread_mutex_lock_with_timer(pthread_mutex_t *mutex) {
  struct timespec start_time, end_time;  clock_gettime(CLOCK_MONOTONIC, &start_time);
  pthread_mutex_lock(mutex);
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  total_wait_time += time_diff(&end_time, &start_time);
}

void __sync_lock_test_and_set_with_timer(int *s_lock, int value) {
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  while (__sync_lock_test_and_set(s_lock, value));
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  total_wait_time += time_diff(&end_time, &start_time);
}

void normal_SortedList_delete(SortedList_t *list, const char *key) {
  SortedListElement_t *element;
  element = SortedList_lookup(list, key);
  if (element == NULL) {
    fprintf(stderr, "Element not found in the list.\n");
    exit(2);
  }
  if (SortedList_delete(element) == 1) {
    fprintf(stderr, "Error deleting an element in the list.\n");
    exit(2);
  }
}

unsigned long hash_function(const char *str) {
  unsigned long sum = 0;
  int i;
  for (i = 0; i < key_length; i++) 
    sum += (i + 1) * str[i];

  return sum;
}

void thread_list_insert(int thread_index) {
  int i, list_index;
  int size = numThreads * numIterations;
  for (i = thread_index; i < size; i += numThreads) {

    list_index = hash_function(elements[i].key) % numLists;

    if (opt_sync) {

      if (lock_type == MUTEX) {
	pthread_mutex_lock_with_timer(&m_locks[list_index]);
	SortedList_insert(&lists[list_index], &elements[i]);
	pthread_mutex_unlock(&m_locks[list_index]);
      }
      else if (lock_type == SPIN_LOCK) {
	__sync_lock_test_and_set_with_timer(&s_locks[list_index], 1);
	SortedList_insert(&lists[list_index], &elements[i]);
	__sync_lock_release(&s_locks[list_index]);
      }
    } 
    else 
      SortedList_insert(&lists[list_index], &elements[i]);
  }
}
   
void thread_list_length() {
  int i, list_length;

  for (i = 0; i < numLists; i++) {

    if (opt_sync) {

      if (lock_type == MUTEX) { 
	pthread_mutex_lock_with_timer(&m_locks[i]);
	list_length = SortedList_length(&lists[i]);
	pthread_mutex_unlock(&m_locks[i]);
      }
      else if (lock_type == SPIN_LOCK) {
	__sync_lock_test_and_set_with_timer(&s_locks[i], 1);
	list_length = SortedList_length(&lists[i]);
	__sync_lock_release(&s_locks[i]);
      }
    }
    else {
      list_length = SortedList_length(&lists[i]);
    }

    if (list_length == -1) {
      fprintf(stderr, "Error getting the length of the list\n");
      exit(2);
    }
  }
}

void thread_list_delete(int thread_index) {
  int i, list_index;
  int size = numThreads * numIterations;
  for (i = thread_index; i < size; i += numThreads) {

    list_index = hash_function(elements[i].key) % numLists;

    if (opt_sync) {

      if (lock_type == MUTEX) {
	pthread_mutex_lock_with_timer(&m_locks[list_index]);
	normal_SortedList_delete(&lists[list_index], elements[i].key);
	pthread_mutex_unlock(&m_locks[list_index]);	
      }
      else if (lock_type == SPIN_LOCK) {
	__sync_lock_test_and_set_with_timer(&s_locks[list_index], 1);
	normal_SortedList_delete(&lists[list_index], elements[i].key);
	__sync_lock_release(&s_locks[list_index]);
      }
    }
    else {
      normal_SortedList_delete(&lists[list_index], elements[i].key);
    }
  }
}

void* list_operation(void *threadInfo) {
  struct ThreadInfo *info = (struct ThreadInfo*) threadInfo;
  thread_list_insert(info->index);
  thread_list_length();
  thread_list_delete(info->index);
  return NULL;
}

char* random_key_generator(int length) {
  int i;
  char *key = (char*) malloc(length + 1);

  for (i = 0; i < length; i++)
    key[i] = 'J' + (rand() % 111);

  key[key_length] = '\0';

  return key;
}

void yield_option_handler(char *optarg) {
  int i;

  for (i = 0; i < (int) strlen(optarg); i++) {
    if (optarg[i] == 'i')
      opt_yield |= INSERT_YIELD;
    else if (optarg[i] == 'd')
      opt_yield |= DELETE_YIELD;
    else if (optarg[i] == 'l')
      opt_yield |= LOOKUP_YIELD;
    else
      invalid_argument();
  }
}

void free_elements() {
  int i;
  int size = numThreads * numIterations;
  for (i = 0; i < size; i++)
    free((char*) elements[i].key);

  free(elements);
}


void malloc_error_check(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "malloc() failed - Error allocating memory\n");
    exit(1);
  }
}

void initialize_mutex_locks() {
  int i;

  m_locks = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * numLists);
  malloc_error_check(m_locks);

  for (i = 0; i < numLists; i++) 
    pthread_mutex_init(&m_locks[i], NULL);
}

void initialize_spin_locks() {
  int i;

  s_locks = (int*) malloc(sizeof(int) * numLists);\
  malloc_error_check(s_locks);

  for (i = 0; i < numLists; i++)
    s_locks[i] = 0;
}

void initialize_locks() {
  if (!opt_sync) return;

  if (lock_type == MUTEX) {
    initialize_mutex_locks();
  }
  else if (lock_type == SPIN_LOCK) {
    initialize_spin_locks();
  }
}

void sync_option_handler(char *optarg) {
  opt_sync = 1;
  char c = optarg[0];
  if (c == 'm') {
    lock_type = MUTEX;
  }
  else if (c == 's') {
    lock_type = SPIN_LOCK;
  }
  else {
   invalid_argument();
  }
}

void yield_tag_handler() {
  strcat(tag, "-");
  if (opt_yield) {

    if (opt_yield & INSERT_YIELD)
      strcat(tag, "i");
    if (opt_yield & DELETE_YIELD)
      strcat(tag, "d");
    if (opt_yield & LOOKUP_YIELD)
      strcat(tag, "l");  
  }
  else {
    strcat(tag, "none");
  }
}

void sync_tag_handler() {
  strcat(tag, "-");
  if (opt_sync) {
    if (lock_type == MUTEX)
      strcat(tag, "m");
    else if (lock_type == SPIN_LOCK)
      strcat(tag, "s");
  } 
  else {
    strcat(tag, "none");
  }
}

char* get_tag() {
  yield_tag_handler();
  sync_tag_handler();
  return tag;
}

void print_result() {
  long long numOperations = numThreads * numIterations * 3;
  long long runtime = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start);
  long long avg_time_per_op = runtime / numOperations;

  long long numLockOperations = numThreads * (numIterations * 2 + 1);
  long long avg_wait_lock_time = total_wait_time / numLockOperations;
  printf("%s,%d,%d,%d,%lld,%lld,%lld,%lld\n", 
	 get_tag(), 
	 numThreads,
	 numIterations,
	 numLists,
	 numOperations,
	 runtime,
	 avg_time_per_op,
	 avg_wait_lock_time);
}

void sigsegv_handler() {
  write(2, "Segmentation fault caught\n", 27);
  exit(2);
}

void list_creation() {
  int i;

  lists = (SortedList_t*) malloc(sizeof(SortedList_t) * numLists);
  malloc_error_check(lists);

  for (i = 0; i < numLists; i++) {
    lists[i].key  = NULL;
    lists[i].prev = &lists[i];
    lists[i].next = &lists[i];
  }
  
  int size = numThreads * numIterations;
  elements = (SortedListElement_t*) malloc(sizeof(SortedListElement_t) * size);
  malloc_error_check(elements);

  srand(time(NULL));
  
  for (i = 0; i < size; i++)
    elements[i].key = random_key_generator(key_length);

  atexit(free_elements);
}

void multi_thread_handler() {

  tids = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
  malloc_error_check(tids);

  int i;

  info = (struct ThreadInfo*) malloc(sizeof(struct ThreadInfo) * numThreads);
  malloc_error_check(info);

  
  for (i = 0; i < numThreads; i++)
    info[i].index = i;


  for (i = 0; i < numThreads; i++)
    pthread_create_handler(&tids[i], NULL, list_operation, (void*) &info[i]);

  for (i = 0; i < numThreads; i++)
    pthread_join_handler(tids[i], NULL);
}

int get_list_length() {
  int i, length;
  int total_length = 0;
  for (i = 0; i < numLists; i++) {
    length = SortedList_length(&lists[i]);
    if (length == -1) {
      fprintf(stderr, "The list is corrupted. Length should be 0\n");
      exit(2);
    }
    total_length += length;
  }
  return total_length;
}


int main(int argc, char **argv) {
  int c;
  static struct option long_options[] = {
    { "threads", required_argument, 0, 't' },
    { "iterations", required_argument, 0, 'i' },
    { "yield", required_argument, 0, 'y' },
    { "sync", required_argument, 0, 's' },
    { "lists", required_argument, 0, 'l' },
    { 0, 0, 0, 0 }
  };

  while (1) {
    c = getopt_long(argc, argv, "", long_options, NULL);
    if (c == -1) break;
    
    switch(c) {
    case 't':
      numThreads = atoi(optarg);
      break;
    case 'i':
      numIterations = atoi(optarg);
      break;
    case 'y':
      yield_option_handler(optarg);
      break;
    case 's':
      sync_option_handler(optarg);
      break;
    case 'l':
      numLists = atoi(optarg);
      break;
    default:
      exit(1);
    }
  }
  
  initialize_locks();

  list_creation();
  signal(SIGSEGV, sigsegv_handler);

  
  clock_gettime(CLOCK_MONOTONIC, &start);
  multi_thread_handler();
  clock_gettime(CLOCK_MONOTONIC, &end);

  
  if (get_list_length() != 0) {
    fprintf(stderr, "List length is not zero.\n");
    exit(2);
  }

  print_result();

  free(lists);
  free(info);
  free(tids);
  free(m_locks);
  free(s_locks);

  exit(0);

}

