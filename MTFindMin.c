//===- MTFindMin.c ------------------------------------------------------------------------------------------------===//
//
// Author: Michael Dorst
//
//===--------------------------------------------------------------------------------------------------------------===//
// CSC 139
// Fall 2019
// Section 2
// Tested on: macOS 10.14 (partially - macOS does not support unnamed POSIX semaphores), CentOS 6.10 (athena)
//===--------------------------------------------------------------------------------------------------------------===//

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>

#define MAX_ARRAY_SIZE 100000000
#define MAX_THREAD_COUNT 16
#define RANDOM_SEED 7665
#define MAX_RANDOM_NUMBER 5000

/**
 * The shared state of all threads - should be instantiated once and passed to each `ThreadInfo` instance.
 * `searchDone` is a binary semaphore that signals when all threads are done, or one finds a zero.
 * `doneThreadCountMutex` is a binary semaphore which acts as a mutex to protect access to `doneThreadCount`.
 * `doneThreadCount` is the number of threads that are done searching.
 */
typedef struct
{
  sem_t searchDone;
  sem_t doneThreadCountMutex;
  size_t doneThreadCount;
  size_t threadCount;
} SharedState;

/**
 * Tracks information about a thread.
 * `done` tracks if the thread is done
 * `data` is the array the thread is searching
 * `minimum` tracks the minimum value found by the thread
 * `region` tracks the region that the thread should search
 * Should be initialized with `initThreadInfo()`.
 */
typedef struct
{
  bool done;
  int const * data;
  int minimum;
  size_t begin_region;
  size_t end_region;
  SharedState * sharedState;
  pthread_t * threadHandle;
} ThreadInfo;

bool allThreadsDone(size_t threadCount, ThreadInfo const * threadInfo);
void cancelAll(pthread_t const * threads, ThreadInfo const * threadInfo, size_t threadCount);
ThreadInfo * computeThreadInfo(int const * data, size_t arraySize, size_t threadCount, SharedState * sharedState);
int findMinInRegion(int const * data, size_t begin, size_t end);
int findMinSequential(int const * data, size_t size);
void * findMinThreaded(void * region);
void * findMinThreadedWithSemaphore(void * threadInfo);
void freeSharedState(SharedState * sharedState);
int * generateInput(size_t size, int indexOfZero);
SharedState initSharedState(size_t threadCount);
void joinAll(pthread_t const * threads, size_t threadCount);
time_t now();
int searchThreadMinima(size_t threadCount, ThreadInfo const * threadInfo);
void startAll(pthread_t * threads, ThreadInfo * threadInfo, size_t threadCount, void * (* f)(void *));
int stoi(char const * str);
time_t timeSince(time_t time);

int main(const int argc, const char ** argv)
{
  if (argc != 4)
  {
    fprintf(stderr, "%s%s%s%s%s",
            "Usage: MTFindMin <array_size> <num_threads> <index_of_zero>\n",
            "array_size: The size of the array to be searched\n",
            "num_threads: The number of threads to use\n",
            "index_of_zero: The index in the array at which to place the zero. ",
            "If -1, no zero will be placed.\n");
    exit(-1);
  }
  int arraySize = stoi(argv[1]);
  if (arraySize <= 0 || arraySize > MAX_ARRAY_SIZE)
  {
    fprintf(stderr, "array_size must be between 1 and %d\n", MAX_ARRAY_SIZE);
    exit(-1);
  }
  int threadCount = stoi(argv[2]);
  if (threadCount > MAX_THREAD_COUNT || threadCount < 1)
  {
    fprintf(stderr, "num_threads must be between 1 and %d\n", MAX_THREAD_COUNT);
    exit(-1);
  }
  int indexOfZero = stoi(argv[3]);
  if (indexOfZero < -1 || indexOfZero >= arraySize)
  {
    fprintf(stderr, "index_of_zero must be between -1 and %d (array_size - 1)\n", arraySize - 1);
    exit(-1);
  }
  int * data = generateInput(arraySize, indexOfZero);
  
  // Sequential:
  time_t startTime = now();
  int min = findMinSequential(data, arraySize);
  printf("Sequential search completed in %ld ms. Min = %d\n", timeSince(startTime), min);
  
  // Threaded with parent waiting for all child threads:
  pthread_t * threads = malloc(threadCount * sizeof(pthread_t));
  ThreadInfo * threadInfo = computeThreadInfo(data, arraySize, threadCount, NULL);
  startTime = now();
  startAll(threads, threadInfo, threadCount, findMinThreaded);
  joinAll(threads, threadCount);
  min = searchThreadMinima(threadCount, threadInfo);
  printf("Threaded search with parent waiting for all children completed in %ld ms. Min = %d\n", timeSince(startTime),
         min);
  free(threadInfo);
  
  // Threaded with parent busy waiting
  threadInfo = computeThreadInfo(data, arraySize, threadCount, NULL);
  startTime = now();
  startAll(threads, threadInfo, threadCount, findMinThreaded);
  while (!allThreadsDone(threadCount, threadInfo))
  {
    if (searchThreadMinima(threadCount, threadInfo) == 0)
    {
      cancelAll(threads, threadInfo, threadCount);
      joinAll(threads, threadCount);
      break;
    }
  }
  min = searchThreadMinima(threadCount, threadInfo);
  printf("Threaded search with parent continually checking on children completed in %ld ms. Min = %d\n",
         timeSince(startTime), min);
  free(threadInfo);
  
  // Threaded with parent waiting on semaphore
  SharedState sharedState = initSharedState(threadCount);
  threadInfo = computeThreadInfo(data, arraySize, threadCount, &sharedState);
  startTime = now();
  startAll(threads, threadInfo, threadCount, findMinThreadedWithSemaphore);
  if (sem_wait(&sharedState.searchDone))
  {
    perror("sem_wait");
    exit(1);
  }
  cancelAll(threads, threadInfo, threadCount);
  joinAll(threads, threadCount);
  min = searchThreadMinima(threadCount, threadInfo);
  printf("Threaded search with parent waiting on a semaphore completed in %ld ms. Min = %d\n", timeSince(startTime),
         min);
  freeSharedState(&sharedState);
  free(threads);
  free(threadInfo);
  free(data);
  return 0;
}

/**
 * Returns `true` if all threads are done.
 */
bool allThreadsDone(size_t threadCount, ThreadInfo const * threadInfo)
{
  size_t i;
  for (i = 0; i < threadCount; ++i)
  {
    if (!threadInfo[i].done) return false;
  }
  return true;
}

/**
 * Calls `pthread_cancel` on every thread in `threads`.
 * @param threads The threads to be cancelled
 * @param threadCount The number of threads in `threads`
 */
void cancelAll(pthread_t const * threads, ThreadInfo const * threadInfo, size_t threadCount)
{
  size_t i;
  for (i = 0; i < threadCount; ++i)
  {
    if (!threadInfo[i].done && pthread_cancel(threads[i]))
    {
      fprintf(stderr, "Tried to cancel a thread that does not exist.\n");
      exit(1);
    }
  }
}

/**
 * Generates a heap allocated array of `ThreadInfo` - one for each thread.
 * @param data The data the threads will be operating on
 * @param arraySize The size of `data`
 * @param threadCount The number of threads (and the number of `ThreadInfo` structs to create)
 * @return An array of size `threadCount` (heap allocated - freeing is the responsibility of the caller)
 */
ThreadInfo * computeThreadInfo(int const * data, size_t arraySize, size_t threadCount, SharedState * sharedState)
{
  ThreadInfo * threadInfo = (ThreadInfo *) malloc(threadCount * sizeof(ThreadInfo));
  size_t i;
  for (i = 0; i < threadCount; ++i)
  {
    threadInfo[i].done = false;
    threadInfo[i].data = data;
    threadInfo[i].minimum = MAX_RANDOM_NUMBER + 1;
    threadInfo[i].begin_region = i * arraySize / threadCount;
    threadInfo[i].end_region = (i + 1) * arraySize / threadCount;
    threadInfo[i].sharedState = sharedState;
  }
  return threadInfo;
}

/**
 * Find the minimum value in the given region of `data`
 * The region to be searched is specified by `[begin, end)`.
 * @param data The data to be searched
 * @param begin The index of the beginning of the region to search (inclusive)
 * @param end The index of the end of the region to search (exclusive)
 * @return The minimum value in the region `[begin, end)` of `data`
 */
int findMinInRegion(int const * data, size_t begin, size_t end)
{
  int min = MAX_RANDOM_NUMBER + 1;
  size_t i;
  for (i = begin; i < end; ++i)
  {
    pthread_testcancel();
    if (data[i] == 0) return 0;
    if (data[i] < min)
    {
      min = data[i];
    }
  }
  return min;
}

/**
 * Find the minimum value in `data`. Single threaded.
 * @param data The data to be searched
 * @param size The size of `data`
 * @return The minimum value in `data`
 */
int findMinSequential(int const * const data, size_t size)
{
  return findMinInRegion(data, 0, size);
}

/**
 * Find the minimum value in `data`. Multi threaded. No early exit.
 * @param region The region of `data` to search
 * @return The minimum value in the specified region of `data`
 */
void * findMinThreaded(void * threadInfo)
{
  ThreadInfo * ti = (ThreadInfo *) threadInfo;
  ti->minimum = findMinInRegion(ti->data, ti->begin_region, ti->end_region);
  ti->done = true;
  return NULL;
}

void * findMinThreadedWithSemaphore(void * threadInfo)
{
  ThreadInfo * ti = (ThreadInfo *) threadInfo;
  ti->minimum = findMinInRegion(ti->data, ti->begin_region, ti->end_region);
  int searchDone;
  if (sem_getvalue(&ti->sharedState->searchDone, &searchDone))
  {
    perror("sem_getvalue");
    exit(1);
  }
  printf("searchDone: %d\n", searchDone);
  if (searchDone == 0) return NULL;
  if (sem_wait(&ti->sharedState->doneThreadCountMutex))
  {
    perror("sem_wait");
    exit(1);
  }
  size_t doneThreadCount = ++ti->sharedState->doneThreadCount;
  if (sem_post(&ti->sharedState->doneThreadCountMutex))
  {
    perror("sem_post");
    exit(1);
  }
  printf("doneThreadCount: %lu\n", doneThreadCount);
  if (doneThreadCount == ti->sharedState->threadCount)
  {
    if (sem_post(&ti->sharedState->searchDone))
    {
      perror("sem_post");
      exit(1);
    }
  }
  return NULL;
}

void freeSharedState(SharedState * sharedState)
{
  if (sem_destroy(&sharedState->searchDone))
  {
    perror("sem_destroy");
  }
  if (sem_destroy(&sharedState->doneThreadCountMutex))
  {
    perror("sem_destroy");
  }
}

/**
 * Creates an array of integers between 1 and `MAX_RANDOM_NUMBER`.
 * Places a single `0` at `indexOfZero`.
 * Note: allocates the array dynamically - freeing is the responsibility of the caller.
 * @param size The size of the array created
 * @param indexOfZero The index at which to place a `0`. If -1, no zero is placed.
 * @returns The created array
 */
int * generateInput(size_t size, int indexOfZero)
{
  if (indexOfZero >= (int) size) return NULL;
  srand(RANDOM_SEED);
  int * array = (int *) malloc(size * sizeof(int));
  size_t i;
  for (i = 0; i < size; ++i)
  {
    array[i] = rand() % MAX_RANDOM_NUMBER + 1;
  }
  if (indexOfZero >= 0)
  {
    array[indexOfZero] = 0;
  }
  return array;
}

SharedState initSharedState(size_t threadCount)
{
  SharedState sharedState;
  // shared = false, value = 1
  if (sem_init(&sharedState.searchDone, false, 1))
  {
    perror("sem_init");
    exit(1);
  }
  if (sem_init(&sharedState.doneThreadCountMutex, false, 1))
  {
    perror("sem_init");
    exit(1);
  }
  sharedState.doneThreadCount = 0;
  sharedState.threadCount = threadCount;
  return sharedState;
}

/**
 * Calls `pthread_join` on every thread in `threads`.
 * @param threads The threads to be joined
 * @param threadCount The number of threads in `threads`
 */
void joinAll(pthread_t const * threads, size_t threadCount)
{
  size_t i;
  for (i = 0; i < threadCount; ++i)
  {
    if (pthread_join(threads[i], NULL))
    {
      perror("pthread_join");
      exit(1);
    }
  }
}

/**
 * Returns the current time in milliseconds.
 */
time_t now()
{
  struct timeb timeBuf;
  ftime(&timeBuf);
  return timeBuf.time * 1000 + timeBuf.millitm;
}

/**
 * Returns the minimum number found by all threads.
 * @param threadCount The number of threads that were searching
 * @param threadInfo An array of size `threadCount` with information about each thread
 */
int searchThreadMinima(size_t threadCount, ThreadInfo const * threadInfo)
{
  int min = MAX_RANDOM_NUMBER + 1;
  size_t i;
  for (i = 0; i < threadCount; i++)
  {
    if (threadInfo[i].minimum == 0)
    {
      return 0;
    }
    if (threadInfo[i].done && threadInfo[i].minimum < min)
    {
      min = threadInfo[i].minimum;
    }
  }
  return min;
}

/**
 * Starts all threads.
 * @param threads An array of thread handles
 * @param threadInfo An array of thread information
 * @param threadCount The number of threads
 * @param f The function to run
 */
void startAll(pthread_t * threads, ThreadInfo * threadInfo, size_t threadCount, void * (* f)(void *))
{
  size_t i;
  for (i = 0; i < threadCount; ++i)
  {
    if (pthread_create(&threads[i], NULL, f, &threadInfo[i]))
    {
      perror("pthread_create");
      exit(1);
    }
  }
}

/**
 * Serves the same function as `atoi()`, but performs a number of checks.
 * Checks that `str` is a numeric value, and also that it is in the range of `int`
 */
int stoi(char const * str)
{
  char * end;
  errno = 0;
  // Use strtol because atoi does not do any error checking
  // 10 is for base-10
  long val = strtol(str, &end, 10);
  // If there was an error, (if the string is not numeric)
  if (end == str || *end != '\0' || errno == ERANGE)
  {
    printf("%s is not a number.\n", str);
    exit(1);
  }
  // Check if val is outside of int range
  if (val < INT_MIN || val > INT_MAX)
  {
    printf("%s has too many digits.\n", str);
    exit(1);
  }
  // Casting long to int is now safe
  return (int) val;
}

/**
 * Returns the number of milliseconds that have passed since `time`.
 */
time_t timeSince(time_t time)
{
  return now() - time;
}
