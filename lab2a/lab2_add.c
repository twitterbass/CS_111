// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495
// SLIPDAYS: 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

pthread_mutex_t lock;
int spin_lock = 0;

int opt_yield = 0;
int opt_sync = 0;

const int NONE = 0;
const int MUTEX = 1;
const int SPIN_LOCK = 2;
const int COMPARE_AND_SWAP = 3;
int lock_type = 0;


char tag[32] = "add";

pthread_t *pthreads;

long long counter = 0;
int numThreads = 1;
int numIterations = 1;
struct timespec start, end;


/////////////////////////////////////
void pthread_create_handler(pthread_t *thread, const pthread_attr_t *attr,
			    void *(*start_routine)(void*), void *arg)
{
  if (pthread_create(thread, attr, start_routine, arg) != 0) {
    fprintf(stderr, "pthread_create() failed - %s\n", strerror(errno));
    exit(1);
  }
}

void pthread_join_handler(pthread_t thread, void **value_ptr) {
  if (pthread_join(thread, value_ptr)!= 0) {
    fprintf(stderr, "pthread_join() failed - %s\n", strerror(errno));
    exit(1);
  }
}
/////////////////////////////////////
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void mutex_add(long long *pointer, long long value) {
  pthread_mutex_lock(&lock);
  add(pointer, value);
  pthread_mutex_unlock(&lock);
}

void spin_lock_add(long long *pointer, long long value) {
  while (__sync_lock_test_and_set(&spin_lock, 1));
  add(pointer, value);
  __sync_lock_release(&spin_lock);
}

void compare_and_swap_add(long long *pointer, long long value) {
  long long old;
  long long new;
  
  do {
    old = *pointer;
    new = old + value;
    
    if (opt_yield)
      sched_yield();
  }
  while (__sync_val_compare_and_swap(pointer, old, new) != old);
}

void sync_option_handler(char* option) {
  char c = option[0];
  
  switch (c) {
  case 'm':
    lock_type = MUTEX;
    break;
    
  case 's':
    lock_type = SPIN_LOCK;
    break;

  case 'c':
    lock_type = COMPARE_AND_SWAP;
    break;

  default:
    fprintf(stderr, "Invalid sync argument: %s\n", strerror(errno));
    exit(1);
  }  
}

char* get_tag() {
  if (opt_yield)
    strcat(tag, "-yield");

  switch (lock_type) {
  
  case 0:
    strcat(tag, "-none");
    break;
  
  case 1:
    strcat(tag, "-m");
    break;
  
  case 2:
    strcat(tag, "-s");
    break;
  
  case 3:
    strcat(tag, "-c");
    break;
  }

  return tag;
}

void* test_driver() {
  int i;
  for (i = 0; i < numIterations; i++) {
    if (lock_type == 0) {
      add(&counter, 1);
      add(&counter, -1);
    }
    else if (lock_type == 1) {
      mutex_add(&counter, 1);
      mutex_add(&counter, -1);
    }
    else if (lock_type == 2) {
      spin_lock_add(&counter, 1);
      spin_lock_add(&counter, -1);
    }
    else if (lock_type == 3) {
      compare_and_swap_add(&counter, 1);
      compare_and_swap_add(&counter, -1);
    }
  }

  return NULL;
}

void multi_thread_handler()  
{
  // Initialize mutex lock
  if (lock_type == MUTEX) 
    pthread_mutex_init(&lock, NULL);

  // Get start time
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  // Allocate memory for threads
  pthreads = (pthread_t *) malloc (sizeof(pthread_t) * numThreads);

  int i;
  for (i = 0; i < numThreads; i++) 
    pthread_create_handler(&pthreads[i], NULL, test_driver, NULL);

  for (i = 0; i < numThreads; i++)
    pthread_join_handler(pthreads[i], NULL);

  // Get end time
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  // Deallocate memory
  free(pthreads);
}

static inline long long get_nanosec_from_timespec(struct timespec* spec) {
  long long ret = spec->tv_sec;
  ret = ret * 1000000000 + spec->tv_nsec;
  return ret;
}

void print_result() {
  long long runtime = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start);
  long long numOperations = numThreads * numIterations * 2;
  long long average_time_per_op = runtime / numOperations;
  printf("%s,%d,%d,%lld,%lld,%lld,%lld\n",
	 get_tag(),
	 numThreads,
	 numIterations,
	 numOperations,
	 runtime,
	 average_time_per_op,
	 counter);
}

int main(int argc, char* argv[]) {
  int c;


  

  static struct option long_options[] = {
    { "threads", required_argument, 0, 't' },
    { "iterations", required_argument, 0, 'i' },
    { "yield", no_argument, 0, 'y' },
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
      opt_yield = 1;
      break;

    case 's':
      opt_sync = 1;
      sync_option_handler(optarg);
      break;

    default:
      fprintf(stderr, "Valid options are: --threads, --iterations\n");
      exit(1);
      break;
    }
  }

  multi_thread_handler();
  print_result();
  exit(0);
}
