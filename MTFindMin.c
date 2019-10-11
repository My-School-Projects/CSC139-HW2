#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7665
#define MAX_RANDOM_NUMBER 5000

long g_ref_time;
int g_data[MAX_SIZE];

int g_thread_count;
// Number of threads that are done at a certain point. Whenever a thread is done, it increments this.
// Used with the semaphore-based solution
int g_done_thread_count;
// The minimum value found by each thread
int g_thread_minima[MAX_THREADS];
// Is this thread done? Used when the parent is continually checking on child threads
bool g_thread_done[MAX_THREADS];

// To notify parent that all threads have completed or one of them found a zero
sem_t completed;
// Binary semaphore to protect the shared variable g_done_thread_count
sem_t mutex;

// Sequential FindMin (no threads)
int find_min_sequential(int size);

// Thread FindMin but without semaphores
void * find_min_parallel_spin(void *param);

// Thread FindMin with semaphores
void * find_min_parallel_sem(void *param);

// Search all thread minima to find the minimum value found in all threads
int search_thread_minima();

void init_shared_variables();

// Generate the input array
void generate_input(int size, int index_for_zero);

// Calculate the indices to divide the array into T divisions, one division per thread
void calculate_indices(int array_size, int thread_count, int **indices);

// Get a random number between min and max
int get_rand(int min, int max);

// Timing functions
long get_millisecond_time(struct timeb timeBuf);

long get_current_time(void);

void set_time(void);

long get_time(void);

int main(int argc, char *argv[])
{
  pthread_t tid[MAX_THREADS];
  pthread_attr_t attr[MAX_THREADS];
  int indices[MAX_THREADS][3];
  int index_of_zero, array_size, min;
  
  // Code for parsing and checking command-line arguments
  if (argc != 4)
  {
    fprintf(stderr, "Invalid number of arguments!\n");
    exit(-1);
  }
  if ((array_size = atoi(argv[1])) <= 0 || array_size > MAX_SIZE)
  {
    fprintf(stderr, "Invalid Array Size\n");
    exit(-1);
  }
  g_thread_count = atoi(argv[2]);
  if (g_thread_count > MAX_THREADS || g_thread_count <= 0)
  {
    fprintf(stderr, "Invalid Thread Count\n");
    exit(-1);
  }
  index_of_zero = atoi(argv[3]);
  if (index_of_zero < -1 || index_of_zero >= array_size)
  {
    fprintf(stderr, "Invalid index for zero!\n");
    exit(-1);
  }
  
  generate_input(array_size, index_of_zero);
  
  calculate_indices(array_size, g_thread_count, (int **) indices);
  
  // Code for the sequential part
  set_time();
  min = find_min_sequential(array_size);
  printf("Sequential search completed in %ld ms. Min = %d\n", get_time(), min);
  
  // Threaded with parent waiting for all child threads
  init_shared_variables();
  set_time();
  
  // Write your code here
  // Initialize threads, create threads, and then let the parent wait for all threads using pthread_join
  // The thread start function is find_min_parallel_spin
  // Don't forget to properly initialize shared variables
  
  min = search_thread_minima();
  printf("Threaded FindMin with parent waiting for all children completed in %ld ms. Min = %d\n", get_time(), min);
  
  // Multi-threaded with busy waiting (parent continually checking on child threads without using semaphores)
  init_shared_variables();
  set_time();
  
  // Write your code here
  // Don't use any semaphores in this part
  // Initialize threads, create threads, and then make the parent continually check on all child threads
  // The thread start function is find_min_parallel_spin
  // Don't forget to properly initialize shared variables
  
  min = search_thread_minima();
  printf("Threaded FindMin with parent continually checking on children completed in %ld ms. Min = %d\n", get_time(),
         min);
  
  
  // Multi-threaded with semaphores
  
  init_shared_variables();
  // Initialize your semaphores here
  
  set_time();
  
  // Write your code here
  // Initialize threads, create threads, and then make the parent wait on the "completed" semaphore
  // The thread start function is ThFindMinWithSemaphore
  // Don't forget to properly initialize shared variables and semaphores using sem_init
  
  
  
  min = search_thread_minima();
  printf("Threaded FindMin with parent waiting on a semaphore completed in %ld ms. Min = %d\n", get_time(), min);
}

// Write a regular sequential function to search for the minimum value in the array g_data
int find_min_sequential(int size)
{

}

// Write a thread function that searches for the minimum value in one division of the array
// When it is done, this function should put the minimum in g_thread_minima[threadNum] and set g_thread_done[threadNum] to true
void * find_min_parallel_spin(void *param)
{
  int threadNum = ((int *) param)[0];
  
}

// Write a thread function that searches for the minimum value in one division of the array
// When it is done, this function should put the minimum in g_thread_minima[threadNum]
// If the minimum value in this division is zero, this function should post the "completed" semaphore
// If the minimum value in this division is not zero, this function should increment g_done_thread_count and
// post the "completed" semaphore if it is the last thread to be done
// Don't forget to protect access to g_done_thread_count with the "mutex" semaphore
void * ThFindMinWithSemaphore(void *param)
{

}

int search_thread_minima()
{
  int i, min = MAX_RANDOM_NUMBER + 1;
  
  for (i = 0; i < g_thread_count; i++)
  {
    if (g_thread_minima[i] == 0)
    {
      return 0;
    }
    if (g_thread_done[i] == true && g_thread_minima[i] < min)
    {
      min = g_thread_minima[i];
    }
  }
  return min;
}

void init_shared_variables()
{
  int i;
  
  for (i = 0; i < g_thread_count; i++)
  {
    g_thread_done[i] = false;
    g_thread_minima[i] = MAX_RANDOM_NUMBER + 1;
  }
  g_done_thread_count = 0;
}

// Write a function that fills the g_data array with random numbers between 1 and MAX_RANDOM_NUMBER
// If indexForZero is valid and non-negative, set the value at that index to zero 
void generate_input(int size, int index_for_zero)
{

}

// Write a function that calculates the right indices to divide the array into threadCount equal divisions
// For each division i, indices[i][0] should be set to the division number i,
// indices[i][1] should be set to the start index, and indices[i][2] should be set to the end index 
void calculate_indices(int array_size, int thread_count, int **indices)
{

}

// Get a random number in the range [x, y]
int get_rand(int x, int y)
{
  int r = rand();
  r = x + r % (y - x + 1);
  return r;
}

long get_millisecond_time(struct timeb timeBuf)
{
  long millisecond_time;
  millisecond_time = timeBuf.time;
  millisecond_time *= 1000;
  millisecond_time += timeBuf.millitm;
  return millisecond_time;
}

long get_current_time(void)
{
  long current_time = 0;
  struct timeb time_buf;
  ftime(&time_buf);
  current_time = get_millisecond_time(time_buf);
  return current_time;
}

void set_time(void)
{
  g_ref_time = get_current_time();
}

long get_time(void)
{
  long current_time = get_current_time();
  return (current_time - g_ref_time);
}
