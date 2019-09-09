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

int numThreads = 1;
int numIterations = 1;
int opt_yield = 0;
int opt_sync = 0;
int key_length = 3;
int s_lock    = 0;

pthread_mutex_t m_lock;

SortedList_t *list;
SortedListElement_t *elements;
pthread_t *tids;
struct ThreadInfo *info;

char tag[100] = "list";
int lock_type = 0;
struct timespec start, end;


void invalid_argument() {
  fprintf(stderr, "Invalid argument\n");  
  exit(1);
}

void pthread_create_handler(pthread_t *thread, pthread_attr_t *attr, 
			    void *(*start_routine)(void*), void *arg) {
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

void normal_SortedList_delete(SortedList_t *list, const char *key) {
  SortedListElement_t *element;
  element = SortedList_lookup(list, key);
  if (element == NULL) {
    fprintf(stderr, "Element not found in the list.\n");
    exit(2);
  }
  if (SortedList_delete(element) == 1) {
    fprintf(stderr, "Error deleting an element in the list\n");
    exit(2);
  }
}

void thread_list_insert(int thread_index) {
  int i;
  int size = numThreads * numIterations;
  for (i = thread_index; i < size; i += numThreads) {
    if (opt_sync) {
      if (lock_type == MUTEX) {
	pthread_mutex_lock(&m_lock);
	SortedList_insert(list, &elements[i]);
	pthread_mutex_unlock(&m_lock);
      }
      else if (lock_type == SPIN_LOCK) {
	while (__sync_lock_test_and_set(&s_lock, 1));
	SortedList_insert(list, &elements[i]);
	__sync_lock_release(&s_lock);
      }
    } else {
      SortedList_insert(list, &elements[i]);
    }
  }
}

void thread_list_length() {
  int length;

  if (opt_sync) {
    if (lock_type == MUTEX) {
      pthread_mutex_lock(&m_lock);
      length = SortedList_length(list);
      pthread_mutex_unlock(&m_lock);
    }
    else if (lock_type == SPIN_LOCK) {
      while (__sync_lock_test_and_set(&s_lock, 1));
      length = SortedList_length(list);
      __sync_lock_release(&s_lock);
    }
  }
  else {
    length = SortedList_length(list);
  }

  if (length == -1) {
    fprintf(stderr, "The list is corrupted. Incorrect length\n");
    exit(2);
  }
}

void thread_list_delete(int thread_index) {
  int i;
  int size = numThreads * numIterations;
  for (i = thread_index; i < size; i += numThreads) {
    if (opt_sync) {
      if (lock_type == MUTEX) {
	pthread_mutex_lock(&m_lock);
	normal_SortedList_delete(list, elements[i].key);
	pthread_mutex_unlock(&m_lock);
      }
      else if (lock_type == SPIN_LOCK) {
	while(__sync_lock_test_and_set(&s_lock, 1));
	normal_SortedList_delete(list, elements[i].key);
	__sync_lock_release(&s_lock);
      }
    }
    else {
      normal_SortedList_delete(list, elements[i].key);
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

void yield_option_handler(const char *optarg) {
  size_t i;
  size_t size = strlen(optarg);
  for (i = 0; i < size; i++) {
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

void sync_option_handler(const char *optarg) {
  opt_sync = 1;
  char c = optarg[0];
  if (c == 'm') {
    lock_type = MUTEX;
    pthread_mutex_init(&m_lock, NULL);
  }
  else if (c == 's') 
    lock_type = SPIN_LOCK;
  else
    invalid_argument();
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

static inline long long get_nanosec_from_timespec(struct timespec* spec) {
  long long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

void print_result() {
  int       numLists      = 1;
  long long numOperations = numThreads * numIterations * 3;
  long long runtime       = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start);
  long long avg_time_per_op = runtime / numOperations;
  printf("%s,%d,%d,%d,%lld,%lld,%lld\n", 
	 get_tag(), 
	 numThreads,
	 numIterations,
	 numLists,
	 numOperations,
	 runtime,
	 avg_time_per_op);
}

void sigsegv_handler() {
  write(2, "Segmentation fault caught\n", 27);
  exit(2);
}

void list_creation() {
  list = (SortedList_t *) malloc(sizeof(SortedList_t));
  
  if (list == NULL) {
    fprintf(stderr, "malloc() error - %s\n", strerror(errno));
    exit(1);
  }
  
  list->key  = NULL;
  list->prev = list;
  list->next = list;

  int i;
  int size = numThreads * numIterations;
  elements = (SortedListElement_t *) malloc (sizeof(SortedListElement_t) * size);
 
  if (elements == NULL) {
    fprintf(stderr, "malloc() error - %s\n", strerror(errno));
    exit(1);
  }
   
  srand(time(NULL));

  for (i = 0; i < size; i++)
    elements[i].key = random_key_generator(key_length);
}


void multi_thread_handler()  {
  int i;
  tids = (pthread_t*) malloc (sizeof(pthread_t) * numThreads);
  if (tids == NULL) {
    fprintf(stderr, "malloc() failed - %s\n", strerror(errno));
    exit(1);
  }

  info = (struct ThreadInfo*) malloc (sizeof(struct ThreadInfo) * numThreads);
  if (info == NULL) {
    fprintf(stderr, "malloc() failed - %s\n", strerror(errno));
    exit(1);
  }

  for (i = 0; i < numThreads; i++)
    info[i].index = i;

  for (i = 0; i < numThreads; i++)
    pthread_create_handler(&tids[i], NULL, list_operation, (void*) &info[i]);

  for (i = 0; i < numThreads; i++)
    pthread_join_handler(tids[i], NULL);
}


int main(int argc, char* argv[]) {
  
  int c;
  static struct option long_options[] = {
    { "threads", required_argument, 0, 't' },
    { "iterations", required_argument, 0, 'i' },
    { "yield", required_argument, 0, 'y' }, 
    { "sync", required_argument, 0, 's' },
    { 0, 0, 0, 0 }
  };

  while (1) {
    c = getopt_long(argc, argv, "", long_options, NULL);
    if (c == -1) break;

    switch (c) {
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
    default:
      fprintf(stderr, "Valid options are: --threads, --iterations, --yield, --sync\n");
      exit(1);
    }
  }

  list_creation();

  signal(SIGSEGV, sigsegv_handler);

  clock_gettime(CLOCK_MONOTONIC, &start);
  multi_thread_handler();  
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  print_result();

  // Deallocate resources
  free(list);
  free(tids);
  free(info);

  int i;
  int size = numThreads * numIterations;
  for (i = 0; i < size; i++)
    free( (char*) elements[i].key);
  free(elements);

  exit(0);

}

