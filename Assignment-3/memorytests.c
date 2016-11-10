#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "mymem.h"

/* performs a randomized test:
  totalSize == the total size of the memory pool, as passed to initmem2
    totalSize must be less than 10,000 * minBlockSize
  fillRatio == when the allocated memory is >= fillRatio * totalSize, a block is freed;
    otherwise, a new block is allocated.
    If a block cannot be allocated, this is tallied and a random block is freed immediately thereafter in the next iteration
  minBlockSize, maxBlockSize == size for allocated blocks is picked uniformly at random between these two numbers, inclusive
  */
void do_randomized_test(int strategyToUse, int totalSize, float fillRatio, int minBlockSize, int maxBlockSize, int iterations)
{
  void * pointers[10000];
  int storedPointers = 0;
  int strategy;
  int lbound = 1;
  int ubound = 4;
  int smallBlockSize = maxBlockSize/10;

  if (strategyToUse>0)
    lbound=ubound=strategyToUse;

  FILE *log;
  log = fopen("tests.log","a");
  if(log == NULL) {
    perror("Can't append to log file.\n");
    return;
  }

  fprintf(log,"Running randomized tests: pool size == %d, fill ratio == %f, block size is from %d to %d, %d iterations\n",totalSize,fillRatio,minBlockSize,maxBlockSize,iterations);

  fclose(log);

  for (strategy = lbound; strategy <= ubound; strategy++)
  {
    double sum_largest_free = 0;
    double sum_hole_size = 0;
    double sum_allocated = 0;
    int failed_allocations = 0;
    double sum_small = 0;
    struct timespec execstart, execend;
    int force_free = 0;
    int i;
    storedPointers = 0;

    initmem(strategy,totalSize);

    clock_gettime(CLOCK_REALTIME, &execstart);

    for (i = 0; i < iterations; i++)
    {
      if ( (i % 10000)==0 )
	    srand ( time(NULL) );

      if (!force_free && (mem_free() > (totalSize * (1-fillRatio))))
      {
	    int newBlockSize = (rand()%(maxBlockSize-minBlockSize+1))+minBlockSize;
	    /* allocate */
	    void * pointer = mymalloc(newBlockSize);
	    if (pointer != NULL)
	      pointers[storedPointers++] = pointer;
	    else
	    {
	      failed_allocations++;
	      force_free = 1;
	    }
      }
      else
      {
        int chosen;
        void * pointer;

        /* free */
        force_free = 0;

        if (storedPointers == 0)
          continue;

        chosen = rand() % storedPointers;
        pointer = pointers[chosen];
        pointers[chosen] = pointers[storedPointers-1];

        storedPointers--;

        myfree(pointer);
      }
      sum_largest_free += mem_largest_free();
      sum_hole_size += (mem_free() / mem_holes());
      sum_allocated += mem_allocated();
      sum_small += mem_small_free(smallBlockSize);
    }//for

    clock_gettime(CLOCK_REALTIME, &execend);

    log = fopen("tests.log","a");
    if(log == NULL) {
      perror("Can't append to log file.\n");
      return;
    }

    fprintf(log,"\t=== %s ===\n",strategy_name(strategy));
    fprintf(log,"\tTest took %.2fms.\n", (execend.tv_sec - execstart.tv_sec) * 1000 + (execend.tv_nsec - execstart.tv_nsec) / 1000000.0);
    fprintf(log,"\tAverage hole size: %f\n",sum_hole_size/iterations);
    fprintf(log,"\tAverage largest free block: %f\n",sum_largest_free/iterations);
    fprintf(log,"\tAverage allocated bytes: %f\n",sum_allocated/iterations);
    fprintf(log,"\tAverage number of small blocks: %f\n",sum_small/iterations);
    fprintf(log,"\tFailed allocations: %d\n",failed_allocations);
    fclose(log);
  }
}

/* run randomized tests against the various strategies with various parameters */
int do_stress_tests(int argc, char **argv)
{
  int strategy = strategyFromString(*(argv+1));

  unlink("tests.log");  // We want a new log file

  do_randomized_test(strategy,10000,0.25,1,1000,10000);
  do_randomized_test(strategy,10000,0.25,1,2000,10000);
  do_randomized_test(strategy,10000,0.25,1000,2000,10000);
  do_randomized_test(strategy,10000,0.25,1,3000,10000);
  do_randomized_test(strategy,10000,0.25,1,4000,10000);
  do_randomized_test(strategy,10000,0.25,1,5000,10000);

  do_randomized_test(strategy,10000,0.5,1,1000,10000);
  do_randomized_test(strategy,10000,0.5,1,2000,10000);
  do_randomized_test(strategy,10000,0.5,1000,2000,10000);
  do_randomized_test(strategy,10000,0.5,1,3000,10000);
  do_randomized_test(strategy,10000,0.5,1,4000,10000);
  do_randomized_test(strategy,10000,0.5,1,5000,10000);

  do_randomized_test(strategy,10000,0.5,1000,1000,10000); /* watch what happens with this test!...why? */

  do_randomized_test(strategy,10000,0.75,1,1000,10000);
  do_randomized_test(strategy,10000,0.75,500,1000,10000);
  do_randomized_test(strategy,10000,0.75,1,2000,10000);

  do_randomized_test(strategy,10000,0.9,1,500,10000);

  return 0; /* you nominally pass for surviving without segfaulting */
}

int main(int argc, char **argv)
{
  if( argc < 2) {
    printf("Usage: mem -test <strategy> | mem -try <arg1> <arg2> ... \n");
    exit(-1);
  }
  else if (!strcmp(argv[1],"-test"))
    return do_stress_tests(argc-1,argv+1);
  else if (!strcmp(argv[1],"-try")) {
    try_mymem(argc-1,argv+1);
    return 0;
  } else {
    printf("Usage: mem -test <strategy> | mem -try <arg1> <arg2> ... \n");
    exit(-1);
  }
}
