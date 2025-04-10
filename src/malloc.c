#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                       */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   /** \TODO Implement best fit here */
   struct _block *bestFit = NULL; // initiaze a pointer for the best fit block
   size_t bestSize = SIZE_MAX; //initialize a size parameter for the best size

   while (curr) //iterate until curr is not null, startign from the head
   {
      if(curr->free == true && curr->size >= size && curr->size < bestSize) // if the cur block is free and size is large enough and smaller than the best size
      { 
         bestFit = curr; // update best fit pointer 
         bestSize = curr->size; //update the best fit size
      }
      *last = curr; // track the last block 
      curr  = curr->next; // and then move to the block
   }
   
   return bestFit; // return the best fit block

#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   /** \TODO Implement worst fit here */
   struct _block *worstFit = NULL; // initiaze a pointer for the worst fit block
   size_t worstSize = 0; //initialize a size parameter for the worst size

   while (curr) //iterate until curr is not null, startign from the head
   {
      if(curr->free == true && curr->size >= size && curr->size > worstSize) // if the cur block is free and size is large enough and larger than the worst size
      { 
         worstFit = curr; // update worst fit pointer 
         worstSize = curr->size; //update the worst fit size
      }
      *last = curr; // track the last block 
      curr  = curr->next; // and then move to the block
   }
   
   return worstFit; // return the worst fit block
#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   /** \TODO Implement next fit here */
   static struct _block *nextFit = NULL; // initiaze a static pointer for the next fit from where the last allocated
   struct _block *return_block = NULL; // init a pointer block to hold the return 

   if(nextFit == NULL) // if nothing has been allocated yet
   {
      curr = heapList; // styart from the head of the list as already have
   }
   else  // if not
   {
      curr = nextFit->next; // then srtart from where we left off, last allocated block
   }

   while (curr) //loop through the curr until true
   {
      if(curr->free == true && curr->size >= size)//if block is free and is large enough
      {
         nextFit = curr; // keep note and update the nextfit last allocated block
         return_block = curr; // store this curr in the returning block
         break;  //break froim loop since already found
      }
      *last = curr; // to keep track of the last visited block
      curr = curr->next; // and then mvoe curr to the next block
   }

   // curr = heapList;
   if(return_block == NULL) //if we did not find the returning block
   {
      curr = heapList; // start again from the head of the heap 
      while(curr != nextFit) // lkeep looping until you find the nextfit
      {
         if(curr->free == true && curr->size >= size) // check if block is free and is large enough
         {
            nextFit = curr; // keep note and update the nextfit last allocated block
            return_block = curr; // store this curr in the returning block
            break;  //break froim loop since already found
         }
         *last = curr; // to keep track of the last visited block
         curr = curr->next; // and then mvoe curr to the next block

         if(curr == NULL) // in the case nothing found
         {
            break; // just break
         }
      }
   }

   return return_block;
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   max_heap += sizeof(struct _block) + size;// update the size of max heap
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }
   num_requested += size;

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */
 

   if(next != NULL && next->size >= size ) // if a block is bigger or equal to the size and free
   {
      size_t leftover_space = next->size - size; //calculate the leftover size after splitting

      if (leftover_space >= (sizeof(struct _block)+4)) // if enough leftover size to fit a block
      {
         struct _block *splitBlock = (struct _block *)((char *)next + sizeof(struct _block) + size); // create a new block in the leftover space, similar to growheap code
         splitBlock->size = leftover_space - sizeof(struct _block);// set the size of the new splitted block

         splitBlock->free = true; //mark the new split block as free
         splitBlock->next = next->next; // put it after the original block since we split it

         next->size = size; // the actual original block gets cut down to whatever size was needed
         next->next = splitBlock;// now the original block will be th new splock

         //update the required counters
         num_blocks++;
         num_splits++;

      }
      num_reuses++;
      


   }

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      num_grows++;
      num_blocks++;
      
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   num_mallocs++;
   next->free = false;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */
   bool next_block_free = (curr->next && curr->next->free); // bool to check if next block is free
   bool prev_block_free = false; // bool to check if prev block is free

   struct _block *prev = heapList; // find the prev block
   struct _block *heap = heapList;//pointer for traversal

   while(heap && heap != curr) // loop until find the block before curr
   {
      prev = heap; // set it to prev
      heap = heap->next; //move to the next block 
   }
 
   if(prev && prev != curr && prev->free) // check if prev is free if there is a prev
   {
      prev_block_free = true; // flag true if it is
   }
 
   if(next_block_free && prev_block_free) // if both prev and next are free
   {
      curr->size += sizeof(struct _block) + curr->next->size; // merge the curr with the next
      curr->next = curr->next->next;
      num_coalesces++;
      num_blocks--;

      prev->size += sizeof(struct _block) + curr->size; // then merge the prev with the curr which is merged with next
      prev->next = curr->next;

      curr = prev; // update the curr to the prev which is the total combined
      num_coalesces++; // increment the coalesces
      num_blocks--;

   }
   else if(next_block_free) // in the case where only the next is free 
   {
      curr->size += sizeof(struct _block) + curr->next->size; // merge the curr with the next
      curr->next = curr->next->next; //basically increment the curr->next
      num_coalesces++; // increment the num_colasces
      num_blocks--;

   }
   else if(prev_block_free) // in the case where only the prev is free
   {
      prev->size += sizeof(struct _block) + curr->size; // merge the prev with the curr
      prev->next = curr->next;
      curr = prev; //make the curr the new block
      num_coalesces++; // incremeent the num_coalesces
      num_blocks--;
   }
   num_frees++;
}

void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc
   size_t totalSize = nmemb * size; //get the total memory needed
   void *allocate = malloc(totalSize); // allocate memorty using malloc
   if(allocate != NULL) // check if not null, if true means allocated
   {
      memset(allocate, 0, totalSize); // zerothe allocated mem
   }
   return allocate; // return the allocated memory block
}

void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc
   void *allocate = NULL;
   if(ptr == NULL) // if the ptr is null, it is basicallyt a malloc
   {
      allocate = malloc(size); // return malloc in this case
   } 
   else if(size ==0) // in the case where the size is 0 
   {
      free(ptr); // this means that there is no size and hence just free the mem
      allocate = NULL; // returtn null in this case
   }
   else
   {
      struct _block *prev = BLOCK_HEADER(ptr); // get the header of the currenbt block

      if(prev->size >= size) // if the block is big enough
      {
         allocate = ptr; // use that block 
      }

      else
      {
         allocate = malloc(size); // allocate a new block using the given size if allcoation needed
         if(allocate != NULL) //in the case where the allocation is null 
         {
            memcpy(allocate, ptr, prev->size); // copy the contents into the new block 
            free(ptr); // free the block memory
         }
      }
   }
 
   return allocate;
   
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwM002= ----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
