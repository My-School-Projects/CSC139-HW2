//===- MTFindMin.c ------------------------------------------------------------------------------------------------===//
//
// Author: Michael Dorst
//
//===--------------------------------------------------------------------------------------------------------------===//
// CSC 139
// Fall 2019
// Section 2
// Tested on:
// + Enigma (Ubuntu 19.04)
//   Architecture:        x86_64
//   CPU op-mode(s):      32-bit, 64-bit
//   Byte Order:          Little Endian
//   Address sizes:       40 bits physical, 48 bits virtual
//   CPU(s):              1
//   On-line CPU(s) list: 0
//   Thread(s) per core:  1
//   Core(s) per socket:  1
//   Socket(s):           1
//   NUMA node(s):        1
//   Vendor ID:           AuthenticAMD
//   CPU family:          23
//   Model:               1
//   Model name:          AMD EPYC 7601 32-Core Processor
//   Stepping:            2
//   CPU MHz:             2199.994
//   BogoMIPS:            4399.98
//   Hypervisor vendor:   KVM
//   Virtualization type: full
//   L1d cache:           64K
//   L1i cache:           64K
//   L2 cache:            512K
//   L3 cache:            16384K
//   NUMA node0 CPU(s):   0
//
// + Athena (CentOS 6.10)
//   Architecture:          i686
//   CPU op-mode(s):        32-bit, 64-bit
//   Byte Order:            Little Endian
//   CPU(s):                4
//   On-line CPU(s) list:   0-3
//   Thread(s) per core:    1
//   Core(s) per socket:    2
//   Socket(s):             2
//   Vendor ID:             GenuineIntel
//   CPU family:            6
//   Model:                 45
//   Model name:            Intel(R) Xeon(R) Silver 4110 CPU @ 2.10GHz
//   Stepping:              2
//   CPU MHz:               2095.078
//   BogoMIPS:              4190.15
//   Hypervisor vendor:     VMware
//   Virtualization type:   full
//   L1d cache:             32K
//   L1i cache:             32K
//   L2 cache:              1024K
//   L3 cache:              11264K
//===--------------------------------------------------------------------------------------------------------------===//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <semaphore.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7665
#define MAX_RANDOM_NUMBER 5000

long gRefTime;
int gData[MAX_SIZE];

int gThreadCount;
// Number of threads that are done at a certain point. Whenever a thread is done, it increments this.
// Used with the semaphore-based solution
int gDoneThreadCount;
// The minimum value found by each thread
volatile int gThreadMin[MAX_THREADS];
// Is this thread done? Used when the parent is continually checking on child threads
volatile bool gThreadDone[MAX_THREADS];

// To notify parent that all threads have completed or one of them found a zero
sem_t completed;
// Binary semaphore to protect the shared variable gDoneThreadCount
sem_t mutex;

// Sequential FindMin (no threads)
int SqFindMin(int size);
// Thread FindMin but without semaphores
void * ThFindMin(void * param);
// Thread FindMin with semaphores
void * ThFindMinWithSemaphore(void * param);
// Search all thread minima to find the minimum value found in all threads
int SearchThreadMin();
void InitSharedVars();
// Generate the input array
void GenerateInput(int size, int indexForZero);
// Calculate the indices to divide the array into T divisions, one division per thread
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]);
// Get a random number between min and max
int GetRand(int min, int max);

// Timing functions
long GetMilliSecondTime(struct timeb timeBuf);
long GetCurrentTime(void);
void SetTime(void);
long GetTime(void);

int stoi(char const * str);
bool allThreadsDone();
void cancelAll(pthread_t * threads);
void joinAll(pthread_t * threads);
void startAll(pthread_t * threads, void * (* threadFunc)(void *), int indices[MAX_THREADS][3]);

int main(int argc, char * argv[])
{
  // Code for parsing and checking command-line arguments
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
  if (arraySize <= 0 || arraySize > MAX_SIZE)
  {
    fprintf(stderr, "array_size must be between 1 and %d\n", MAX_SIZE);
    exit(-1);
  }
  gThreadCount = stoi(argv[2]);
  if (gThreadCount > MAX_THREADS || gThreadCount < 1)
  {
    fprintf(stderr, "num_threads must be between 1 and %d\n", MAX_THREADS);
    exit(-1);
  }
  gThreadCount = atoi(argv[2]);
  if (gThreadCount > MAX_THREADS || gThreadCount <= 0)
  {
    fprintf(stderr, "Invalid Thread Count\n");
    exit(-1);
  }
  int indexForZero = stoi(argv[3]);
  if (indexForZero < -1 || indexForZero >= arraySize)
  {
    fprintf(stderr, "index_of_zero must be between -1 and %d (array_size - 1)\n", arraySize - 1);
    exit(-1);
  }
  
  GenerateInput(arraySize, indexForZero);
  
  int indices[MAX_THREADS][3];
  CalculateIndices(arraySize, gThreadCount, indices);
  
  // Code for the sequential part
  SetTime();
  int min = SqFindMin(arraySize);
  printf("Sequential search completed in %ld ms. Min = %d\n", GetTime(), min);
  
  
  // Threaded with parent waiting for all child threads
  InitSharedVars();
  SetTime();
  
  // Write your code here
  // Initialize threads, create threads, and then let the parent wait for all threads using pthread_join
  // The thread start function is ThFindMin
  // Don't forget to properly initialize shared variables
  pthread_t threads[MAX_THREADS];
  startAll(threads, ThFindMin, indices);
  joinAll(threads);
  min = SearchThreadMin();
  printf("Threaded FindMin with parent waiting for all children completed in %ld ms. Min = %d\n", GetTime(), min);
  
  // Multi-threaded with busy waiting (parent continually checking on child threads without using semaphores)
  InitSharedVars();
  SetTime();
  
  // Write your code here
  // Don't use any semaphores in this part
  // Initialize threads, create threads, and then make the parent continually check on all child threads
  // The thread start function is ThFindMin
  // Don't forget to properly initialize shared variables
  startAll(threads, ThFindMin, indices);
  while (!allThreadsDone())
  {
    if (SearchThreadMin() == 0)
    {
      cancelAll(threads);
      break;
    }
  }
  joinAll(threads);
  min = SearchThreadMin();
  printf("Threaded FindMin with parent continually checking on children completed in %ld ms. Min = %d\n", GetTime(),
         min);
  
  // Multi-threaded with semaphores
  sem_init(&completed, false, 0);
  sem_init(&mutex, false, 1);
  
  InitSharedVars();
  // Initialize your semaphores here
  
  SetTime();
  
  // Write your code here
  // Initialize threads, create threads, and then make the parent wait on the "completed" semaphore
  // The thread start function is ThFindMinWithSemaphore
  // Don't forget to properly initialize shared variables and semaphores using sem_init
  startAll(threads, ThFindMinWithSemaphore, indices);
  if (sem_wait(&completed))
  {
    perror("sem_wait");
    exit(1);
  }
  cancelAll(threads);
  joinAll(threads);
  min = SearchThreadMin();
  printf("Threaded FindMin with parent waiting on a semaphore completed in %ld ms. Min = %d\n", GetTime(), min);
  return 0;
}

bool allThreadsDone()
{
  int i;
  for (i = 0; i < gThreadCount; ++i)
  {
    if (!gThreadDone[i]) return false;
  }
  return true;
}

void cancelAll(pthread_t * threads)
{
  int i;
  for (i = 0; i < gThreadCount; ++i)
  {
    pthread_cancel(threads[i]);
  }
}

void joinAll(pthread_t * threads)
{
  int i;
  for (i = 0; i < gThreadCount; ++i)
  {
    if (pthread_join(threads[i], NULL))
    {
      perror("pthread_join");
      exit(1);
    }
  }
}

void startAll(pthread_t * threads, void * (* threadFunc)(void *), int indices[MAX_THREADS][3])
{
  int i;
  for (i = 0; i < gThreadCount; ++i)
  {
    if (pthread_create(&threads[i], NULL, threadFunc, indices[i]))
    {
      perror("pthread_create");
      exit(1);
    }
  }
}

int findMinInRegion(int beg, int end)
{
  int min = MAX_RANDOM_NUMBER + 1;
  int i;
  for (i = beg; i <= end; ++i)
  {
    pthread_testcancel();
    if (gData[i] == 0) return 0;
    if (gData[i] < min)
    {
      min = gData[i];
    }
  }
  return min;
}

// Write a regular sequential function to search for the minimum value in the array gData
int SqFindMin(int size)
{
  return findMinInRegion(0, size-1);
}

// Write a thread function that searches for the minimum value in one division of the array
// When it is done, this function should put the minimum in gThreadMin[threadNum] and set gThreadDone[threadNum] to true    
void * ThFindMin(void * param)
{
  int * info = (int *) param;
  int threadNum = info[0];
  int begin = info[1];
  int end = info[2];
  gThreadMin[threadNum] = findMinInRegion(begin, end);
  gThreadDone[threadNum] = true;
  return NULL;
}

// Write a thread function that searches for the minimum value in one division of the array
// When it is done, this function should put the minimum in gThreadMin[threadNum]
// If the minimum value in this division is zero, this function should post the "completed" semaphore
// If the minimum value in this division is not zero, this function should increment gDoneThreadCount and
// post the "completed" semaphore if it is the last thread to be done
// Don't forget to protect access to gDoneThreadCount with the "mutex" semaphore
void * ThFindMinWithSemaphore(void * param)
{
  int * info = (int *) param;
  int threadNum = info[0];
  int begin = info[1];
  int end = info[2];
  gThreadMin[threadNum] = findMinInRegion(begin, end);
  if (gThreadMin[threadNum] == 0)
  {
    sem_post(&completed);
  }
  else
  {
    sem_wait(&mutex);
    if (++gDoneThreadCount == gThreadCount)
    {
      sem_post(&completed);
    }
    sem_post(&mutex);
  }
  return NULL;
}

int SearchThreadMin()
{
  int i, min = MAX_RANDOM_NUMBER + 1;
  
  for (i = 0; i < gThreadCount; i++)
  {
    if (gThreadMin[i] == 0)
    {
      return 0;
    }
    if (gThreadMin[i] < min)
    {
      min = gThreadMin[i];
    }
  }
  return min;
}

void InitSharedVars()
{
  int i;
  
  for (i = 0; i < gThreadCount; i++)
  {
    gThreadDone[i] = false;
    gThreadMin[i] = MAX_RANDOM_NUMBER + 1;
  }
  gDoneThreadCount = 0;
}

// Write a function that fills the gData array with random numbers between 1 and MAX_RANDOM_NUMBER
// If indexForZero is valid and non-negative, set the value at that index to zero 
void GenerateInput(int size, int indexForZero)
{
  if (indexForZero >= (int) size)
  {
    fprintf(stderr, "Index for zero too large");
    exit(1);
  }
  srand(RANDOM_SEED);
  int i;
  for (i = 0; i < size; ++i)
  {
    gData[i] = rand() % MAX_RANDOM_NUMBER + 1;
  }
  if (indexForZero >= 0)
  {
    gData[indexForZero] = 0;
  }
}

// Write a function that calculates the right indices to divide the array into thrdCnt equal divisions
// For each division i, indices[i][0] should be set to the division number i,
// indices[i][1] should be set to the start index, and indices[i][2] should be set to the end index 
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3])
{
  int i = 0;
  for (i = 0; i < thrdCnt; ++i)
  {
    indices[i][0] = i;
    indices[i][1] = (i * arraySize) / thrdCnt;
    indices[i][2] = ((i + 1) * arraySize) / thrdCnt - 1;
  }
}

// Get a random number in the range [x, y]
int GetRand(int x, int y)
{
  int r = rand();
  r = x + r % (y - x + 1);
  return r;
}

long GetMilliSecondTime(struct timeb timeBuf)
{
  long mliScndTime;
  mliScndTime = timeBuf.time;
  mliScndTime *= 1000;
  mliScndTime += timeBuf.millitm;
  return mliScndTime;
}

long GetCurrentTime(void)
{
  long crntTime = 0;
  struct timeb timeBuf;
  ftime(&timeBuf);
  crntTime = GetMilliSecondTime(timeBuf);
  return crntTime;
}

void SetTime(void)
{
  gRefTime = GetCurrentTime();
}

long GetTime(void)
{
  long crntTime = GetCurrentTime();
  return (crntTime - gRefTime);
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
