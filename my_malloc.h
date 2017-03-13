#include <stdlib.h>
typedef struct heapblock {
  int in_use; // 0 when a block is in some free list; true otherwise
  unsigned size; // size of this block;
  unsigned prev_size; // size of previous block
  struct heapblock* prev; 
  struct heapblock* next;
} heap_block;

#ifndef EXPOSE_REAL_MALLOC
#define malloc my_malloc
#define free my_free
#endif

void *my_malloc(size_t size);
void my_free(void *p);
