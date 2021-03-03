#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


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
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
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
  while (curr && !(curr->free && curr->size >= size)) 
  {
    *last = curr;
    curr = curr->next;
  }
#endif

#if defined BEST && BEST == 0
  struct _block *retBlock = NULL;
  struct _block *prevBlock = NULL;
  while(curr != NULL)
  {
    if(curr->free == 1 && curr->size >= size)
    { 
      if(retBlock == NULL || (retBlock && retBlock->size > curr->size))
      {
        retBlock = curr;
        prevBlock = *last;
      }
    }
    *last = curr;
    curr = curr->next;
  }
  if(retBlock)
  {
    curr = retBlock;
    *last = prevBlock;
  }
#endif

#if defined WORST && WORST == 0
  struct _block *retBlock = NULL;
  struct _block *prevBlock = NULL;
  while(curr != NULL)
  {
    if(curr->free == 1 && curr->size >= size)
    {
      if(retBlock == NULL || (retBlock && retBlock->size < curr->size))
      {
        retBlock = curr;
        prevBlock = *last;
      }
    }
    *last = curr;
    curr = curr->next;
  }
  if(retBlock)
  {
    curr = retBlock;
    *last = prevBlock;
  }
#endif

#if defined NEXT && NEXT == 0
  struct _block *start = NULL;
  if(*last && (*last)->next)
  {
    start = (*last)->next->next;
  }
  curr = start;
  while(curr && !(curr->free && curr->size >= size))
  {
    *last = curr;
    curr = curr->next;
  }
  if(curr == NULL)
  {
    curr = heapList;
    while(curr && !(curr->free && curr->size >= size))
    {
      *last = curr;
      curr = curr->next;
    }
  }
#endif
  if(curr != NULL)
  {
    num_blocks--;
  }
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

   /* Attach new _block to prev _block */
   if(last) 
   {
      last->next = curr;
   }

   /* Update _block metadata */
  num_grows++;
  curr->size = size;
  curr->next = NULL;
  curr->free = false;
  // if(curr)
  //   printf("Block is: %p\n", curr);
  if(last)
  {
    curr->prev = last;
  }
  else
    curr->prev = NULL;
  max_heap = max_heap + size;
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
    // num_mallocs--;
    atexit_registered = 1;
    atexit( printStatistics );
  }
  num_requested = num_requested + size;

  /* Align to multiple of 4 */
  size = ALIGN4(size);

  /* Handle 0 size */
  if (size == 0) 
  {
    return NULL;
  }

  /* Look for free _block */
  struct _block *last = heapList;
  struct _block *next = findFreeBlock(&last, size);

  /* TODO: Split free _block if possible */
  
  struct _block* temp = NULL;
  // Split the blocks
  if(next && next->size - size > 2 * sizeof(struct _block))
  { 

    temp = BLOCK_DATA(next) + (size/sizeof(struct _block));
    temp->size = next->size - size - sizeof(struct _block);
    temp->free = true;
    temp->prev = next;
    temp->next = next->next;
    if(next->next)
    {
      next->next->prev = temp;
    }
    next->size = size;
    next->next = temp;
    num_blocks++;
    num_splits++;
  }
  /* Could not find free _block, so grow heap */
  if (next == NULL) 
  {
    next = growHeap(last, size);
  }
  else
  {
    num_reuses++;
  }
  /* Could not find free _block or grow heap, so just return NULL */
  if (next == NULL) 
  {
    return NULL;
  }
   
  /* Mark _block as in use */
  next->free = false;

  num_mallocs++;
  /* Return data address associated with _block */
  return BLOCK_DATA(next);
}

void* calloc(size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;
  struct _block *ptr = malloc(total_size);

  memset(ptr, 0, size);
  return ptr;
}


void* realloc(void *ptr, size_t size)
{
  struct _block *header = BLOCK_HEADER(ptr);
  size_t req_size = size - header->size;
  if(req_size <= 0)
  {
    return BLOCK_DATA(header);
  }
  // if given ptr is the last block
  // expand the block with the missing size.
  // return back the same pointer.
  if(header->next == NULL)
  {
    struct _block *new_ptr = malloc(req_size - sizeof(struct _block));
    header->next = new_ptr->next;
    header->size = header->size + new_ptr->size + sizeof(struct _block);
    return BLOCK_DATA(header); 
  }
  // if ptr has free block next to it with the required size
  // take req_size from free block
  // reduce the size of free block
  else if(header->next && header->next->free && 
          header->next->size > req_size)
  {
    struct _block *new_ptr = header->next;
    header->size = size;
    header->next = BLOCK_DATA(header) + size/sizeof(struct _block);
    header->next->size = new_ptr->size - req_size;
    header->next->free = 1;
    header->next->prev = header;
    header->next->next = new_ptr->next;
    return BLOCK_DATA(header);
  }
  // none of the above conditions valid
  // create new block with the size
  // copy the data from the previous block
  // free the previous block;
  else
  {
    struct _block *new_ptr = malloc(size);
    if(new_ptr)
    {
      memcpy(new_ptr, BLOCK_DATA(header), header->size);
      free(BLOCK_DATA(header));
    }
    return new_ptr;
  }
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
  num_blocks++;
  num_frees++;
  // struct _block * temp = curr;
  //check if the blocks ahead are free
  // merge all free blocks
  
  while(curr && curr->next && curr->next->free)
  {
    curr->size = curr->size + curr->next->size;// + sizeof(struct _block);
    curr->next = curr->next->next;
    num_blocks--;
    num_coalesces++;
  }
  // check if blocks behind are free
  // merge all free blocks
  while(curr && curr->prev && curr->prev->free)
  {
    curr->prev->size = curr->prev->size + curr->size + sizeof(struct _block);
    curr->prev->next = curr->next;
    curr = curr->prev;
    num_blocks--;
    num_coalesces++;
  }
  /* TODO: Coalesce free _blocks if needed */
}


/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
