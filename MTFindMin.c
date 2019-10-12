//===- MTFindMin.c ------------------------------------------------------------------------------------------------===//
//
// Author: Michael Dorst
//
//===--------------------------------------------------------------------------------------------------------------===//
// CSC 139
// Fall 2019
// Section 2
// Tested on: macOS 10.14, CentOS 6.10 (athena)
//===--------------------------------------------------------------------------------------------------------------===//

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>

#define MAX_ARRAY_SIZE 100000000
#define MAX_THREAD_COUNT 16
#define RANDOM_SEED 7665
#define MAX_RANDOM_NUMBER 5000

/**
 * Tracks information about a thread
 * `done` tracks if the thread is done
 * `minimum` tracks the minimum value found by the thread
 * Should be initialized with `initThreadInfo()`.
 */
typedef struct
{
  bool done;
  int minimum;
} ThreadInfo;

size_t * calculateIndices(size_t arraySize, size_t divisions)
int * generateInput(size_t size, int indexOfZero);
ThreadInfo initThreadInfo();
time_t now();
int stoi(const char * str);

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
  size_t * indices = calculateIndices(arraySize, threadCount);
  time_t startTime = now();
  free(data);
  free(indices);
}

int findMinInRegion(int * data, size_t start, size_t end)
{
  int min = MAX_RANDOM_NUMBER + 1;
  size_t i;
  for (i = start; i <= end; i++)
  {
    if (data[i] == 0) return 0;
    if (data[i] < min)
    {
      min = data[i];
    }
  }
  return min;
}

int searchThreadMinima(int threadCount, ThreadInfo * threadInfo)
{
  int i, min = MAX_RANDOM_NUMBER + 1;
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

/**
 * Logically splits an array into equal regions, and returns the indices of the end of each region.
 * Note: the "equal regions" may be different in size by at most one element.
 * @param arraySize The size of the array
 * @param regions The number of regions
 * @return The calculated indices (heap allocated - freeing is the responsibility of the caller)
 */
size_t * calculateIndices(size_t arraySize, size_t regions)
{
  size_t * indices = (size_t *) malloc(regions * sizeof(size_t));
  int i;
  for (i = 1; i <= regions; ++i)
  {
    indices[i] = i * arraySize / regions - 1;
  }
  return indices;
}

/**
 * Get the begin index of a division
 * @param indices The array of indices to query
 * @param division The division you want to get the begin index of
 * @return The index of the beginning of `division` in `array`
 */
int beginIndex(int * indices, size_t division)
{
  return indices[2 * division];
}

/**
 * Get the end index of a division
 * @param indices The array of indices to query
 * @param division The division you want to get the end index of
 * @return The index of the end of `division` in `array`
 */
int endIndex(int * indices, size_t division)
{
  return indices[2 * division + 1];
}

time_t now()
{
  struct timeb timeBuf;
  ftime(&timeBuf);
  return timeBuf.time * 1000 + timeBuf.millitm;
}

time_t timeSince(time_t time)
{
  return now() - time;
}

/**
 * Returns a `ThreadInfo` with default values.
 */
ThreadInfo initThreadInfo()
{
  ThreadInfo threadInfo;
  threadInfo.done = false;
  threadInfo.minimum = MAX_RANDOM_NUMBER + 1;
  return threadInfo;
}

/**
 * Serves the same function as `atoi()`, but performs a number of checks.
 * Checks that `str` is a numeric value, and also that it is in the range of `int`
 */
int stoi(const char * str)
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
