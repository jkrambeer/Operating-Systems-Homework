#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>
#include <stdint.h>

/********************
 * Joseph Krambeer
 * CISC 310
 * Assignment 3
 ********************/

/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */
struct memoryList
{
  // doubly-linked list
  struct memoryList *last;
  struct memoryList *next;

  int size;            // How many bytes in this block?
  char alloc;          // 1 if this block is allocated,
                       // 0 if this block is free.
  void *ptr;           // location of block in memory pool.
};

strategies myStrategy = NotSet;    // Current strategy


size_t mySize;//size of memory pool (bytes)
void *myMemory = NULL;//actual memory pool

static struct memoryList *head;//start of linked list
static struct memoryList *next;//used to indicate link to start next stategy at

void split_block(struct memoryList *trav, int req);


/* initmem must be called prior to mymalloc and myfree.

   initmem may be called more than once in a given exeuction;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.

   strategy must be one of the following:
		- "best" (best-fit)
		- "worst" (worst-fit)
		- "first" (first-fit)
		- "next" (next-fit)
   sz specifies the number of bytes that will be available, in total, for all mymalloc requests.
*/

void initmem(strategies strategy, size_t sz)
{
    struct memoryList *current = head;
    struct memoryList *temp;
	myStrategy = strategy;

	/* all implementations will need an actual block of memory to use */
	mySize = sz;

	/* in case this is not the first time initmem2 is called */
	if (myMemory != NULL){
		free(myMemory);
	}

	/* Release any other memory previously used for bookkeeping during re-initialization */
    while(current != NULL){
        temp = current->next;//get next element of the list
        free(current);       //free the current element of the list
        current = temp;      //update the current element
    }

	myMemory = malloc(sz);//initialize memory

	/* Initialize memory management structure. */
    head = malloc(sizeof(struct memoryList));
    head->size = sz;     //first link is encapsulates the entire memory block to start
    head->alloc = 0;     //not allocated at the beginning
    head->ptr = myMemory;//the blocks starts at the same spot memory starts
    head->last = NULL;   //no element before start of list
    head->next = NULL;   //no element after head of list as it is only element
    next = head;         //set the point to start next strategy at the current head
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1
 */
void *mymalloc(size_t requested)
{
	assert((int)myStrategy > 0);
    if(requested < 1){ return NULL; }//if less than 1 byte, return NULL

    size_t sizeDif;
    void *new_mem = NULL;
    struct memoryList *current = head;
    struct memoryList *usedBlock = NULL;

	switch (myStrategy)
    {
	  case NotSet:
	    return NULL;

      /*find first available block of request size
        and put new memory there */
	  case First:
        while(current != NULL) {
            if( !(current->alloc) && (current->size >= requested)) {
                usedBlock = current;
                break;
            }
            current = current->next;
        }
	    break;

      /*find block with a size closest to size
        and put new memory there */
	  case Best:
        sizeDif = mySize;
        while(current != NULL) {
            //find unallocated block with the smallest size difference
            //between it and the requested size
            if( !(current->alloc) && (current->size >= requested) &&
                 ((current->size - requested) < sizeDif) ) {
                sizeDif = current->size;
                usedBlock = current;
            }
            current = current->next;
        }
        break;

      /*find block with a size farthest away from
        requested size and put new memory there   */
	  case Worst:
        sizeDif = 0;
        while(current != NULL) {
            //find unallocated block with the greatest size difference
            //between it and the requested size
            if( !(current->alloc) && (current->size >= requested) &&
                 ((current->size - requested) > sizeDif) ) {
                sizeDif = current->size;
                usedBlock = current;
            }
            current = current->next;
        }
        break;

      /*find first block of requested size found
        after the block the last sucessful myMalloc used */
	  case Next:
        current = next;//start at last allocated block instread of head
        while(current != NULL) {
            if( !(current->alloc) && (current->size >= requested)) {
                usedBlock = current;
                break;
            }
            current = current->next;
        }
        //if an open spot was not found after the last allocated block
        //search starting at head for an open spot
        if(usedBlock == NULL) {
            current = head;
            while(current != next) {
                if( !(current->alloc) && (current->size >= requested)) {
                    usedBlock = current;
                    break;
                }
                current = current->next;
            }
        }//if not found at end
	    break;

   	}//switch case

    if(usedBlock != NULL) {
        usedBlock->alloc = 1;             //block is now allocated
        split_block(usedBlock, requested);//split memory
        new_mem = usedBlock->ptr;         //set return pointer to newly allocated memory
        next = usedBlock;                 //update next pointer
    }
	return new_mem;
}//myalloc

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block)
{
    int wasNext = 0;//boolean value to tell if next was changed in free
    struct memoryList *temp = head;
    struct memoryList *memBlock = NULL;

    //try to find an allocated block with same pointer as passed pointer
	while(temp != NULL) {
        if(temp->alloc && (temp->ptr == block)) {
            memBlock = temp;
            break;
        }
        temp = temp->next;
    }

    if(memBlock == NULL){ return; }//if no blocks match, no block can be free'd
    memBlock->alloc = 0;//mark memory as free

    //merge with unallocated block after freed block, after block merged with current block
    if(memBlock->next != NULL && !(memBlock->next->alloc)) {
        wasNext = (memBlock->next == next);    //check if next element is next pointer
        temp = memBlock->next;                 //hold temp reference to link that is being merged
        memBlock->size += memBlock->next->size;//add on next block's size when merging
        memBlock->next = memBlock->next->next; //update the next link to skip over merged block
        if(memBlock->next != NULL) {
            memBlock->next->last = memBlock;   //update the last of next block if it exists
        }
        free(temp);                            //free link that was merged
        if(wasNext){ next = memBlock; }        //update the next pointer if it was free'd
    }//if merge with next block

    //merge with unallocated block before freed block, current block merged into before block
    if(memBlock->last != NULL && !(memBlock->last->alloc)) {
        wasNext = (memBlock == next);  //check if current element is next pointer
        temp = memBlock->last;         //hold temp reference to link that is being merged
        temp->size += temp->next->size;//add on next block's size when merging
        temp->next = temp->next->next; //update the next link to skip over merged block
        if(temp->next != NULL) {
            temp->next->last = temp;   //update next of last block if it exists
        }
        free(memBlock);                //free link that was merged
        if(wasNext){ next = temp; }    //update the next pointer if it was free'd
    }//if merge with prev block

}//myfree

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when we refer to "memory" here, we mean the
 * memory pool this module manages via initmem/mymalloc/myfree.
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes()
{
	struct memoryList *current = head;
    int count = 0;

    while(current != NULL) {
        if( !(current->alloc)) {
            count++;//increment count as another free block was found
        }
        current = current->next;
    }
    return count;
}//mem_holes

/* Get the number of bytes allocated */
int mem_allocated()
{
	struct memoryList *current = head;
	int used_bytes = 0;

	while(current != NULL) {
		if( current->alloc ) {
			used_bytes += current->size;//add on amount this allocated block used
		}
		current = current->next;
	}
	return used_bytes;
}//mem_allocated

/* Number of non-allocated bytes */
int mem_free()
{
	struct memoryList *current = head;
	int free_bytes = 0;

	while(current != NULL) {
		if( !(current->alloc)) {
			free_bytes += current->size;//add size onto total of free memory
		}
		current = current->next;
	}
	return free_bytes;
}//mem_free

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free()
{
	struct memoryList *current = head;
    int largest = 0;

	while(current != NULL) {
		if( !(current->alloc) && (largest < current->size) ) {
			largest = current->size;//update when a free block is larger than the current largest block
		}
		current = current->next;
	}
	return largest;
}//mem_largest_free

/* Number of free blocks smaller than "size" bytes. */
int mem_small_free(int size)
{
	struct memoryList *current = head;
	int count = 0;

	while(current != NULL) {
		if( !(current->alloc) && (current->size < size) ) {
			count++;//update count when block is smaller than size and isn't allocated
		}
		current = current->next;
	}
	return count;
}//mem_small_free

/* Check if a give pointer is allocated */
char mem_is_alloc(void *ptr)
{
    struct memoryList *current = head;
	uintptr_t address = (uintptr_t)ptr;//cast ptr to unsigned int for comparison
	uintptr_t start, end;//used to indicated start/end of mem blocks

	while(current != NULL) {
		start = (uintptr_t) current;//the address this block starts at
		end = (uintptr_t) current+(current->size)-1;//calculate the address this block ends at
		if(current->alloc && (address >= start && address <= end) ) {
			return 1;//the pointer is within an allocated block, so return true
		}
        current = current->next;
	}
	return 0;//if the pointer isn't within the bounds of any allocated memory space, return false
}//mem_is_alloc

/*
 * Feel free to use these functions, but do not modify them.
 * The test code uses them, but you may ind them useful.
 */

/*
 * split_block()
 *
 * Split a block that contains unallocated memory into two blocks.
 * The first (existing) block is marked as 'allocated' and given a new
 * size.
 * The second (new) block is marked as 'unallocated' and contains the
 * address and size of that portion of memory that remains after the
 * first (existing) block is marked as 'allocated'.
 *
 * This function will modify the existing linked list element to indicate
 * that the memory, which had previously been considered 'unallocated' is
 * now considered allocated.  The size of the block is reduced to the size
 * of the requested memory block.
 *
 * A new linked list element is created and inserted immediately after the
 * existing (now allocated) linked list element.
 */

void split_block(struct memoryList *trav, int req)
{
    struct memoryList *temp = NULL;

    if (trav->size > req) {
        temp = malloc(sizeof(struct memoryList));
        temp->last = trav;
        temp->next = trav->next;
        if (trav->next != NULL)
            trav->next->last = temp;

        trav->next = temp;
        temp->size = trav->size - req;
        temp->ptr = trav->ptr + req;
        temp->alloc = 0;
        trav->size = req;
    }

}

//Returns a pointer to the memory pool.
void *mem_pool()
{
	return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total()
{
	return mySize;
}


// Get string name for a strategy.
char *strategy_name(strategies strategy)
{
	switch (strategy)
	{
		case Best:
			return "best";
		case Worst:
			return "worst";
		case First:
			return "first";
		case Next:
			return "next";
		default:
			return "unknown";
	}
}

// Get strategy from name.
strategies strategyFromString(char * strategy)
{
	if (!strcmp(strategy,"best"))
	{
		return Best;
	}
	else if (!strcmp(strategy,"worst"))
	{
		return Worst;
	}
	else if (!strcmp(strategy,"first"))
	{
		return First;
	}
	else if (!strcmp(strategy,"next"))
	{
		return Next;
	}
	else
	{
		return 0;
	}
}


/*
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory()
{
	struct memoryList *current = head;
	int i = 0;//block ID
	while(current != NULL) {
		printf("\n-------Block %d-------\n",i);
		printf("-Size    : %d\n" \
		       "-Status  : %s\n" \
               "-Pointer : %p\n" \
               "-Last    : %p\n" \
               "-This    : %p\n" \
               "-Next    : %p\n",
		        current->size, current->alloc?"allocated":"free",
                current->ptr, current->last, current, current->next);
		printf("---------------------\n");
		current = current->next;//get next element in list
		i++;
	}
	return;
}


/* Use this function to track memory allocation performance.
 * This function does not depend on your implementation,
 * but on the functions you wrote above.
 */
void print_memory_status()
{
	printf("%d out of %d bytes allocated.\n",mem_allocated(),mem_total());
	printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n",mem_free(),mem_holes(),mem_largest_free());
	printf("Average hole size is %f.\n\n",((float)mem_free())/mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
    strategies strat;
	void *a, *b, *c, *d, *e;
	if(argc > 1)
	  strat = strategyFromString(argv[1]);
	else
	  strat = First;

	/* A simple example.
	   Each algorithm should produce a different layout. */
    initmem(strat,500);

	a = mymalloc(100);
	b = mymalloc(100);
	c = mymalloc(100);
	myfree(b);
	d = mymalloc(50);
	myfree(a);
	e = mymalloc(25);

	print_memory();
	print_memory_status();

    //to make the compile stop warning that c,d,e aren't used
    c = c;d = d;e = e;
}
