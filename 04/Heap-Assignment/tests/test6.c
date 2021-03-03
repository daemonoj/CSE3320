#include <stdlib.h>
#include <stdio.h>

int main()
{
  printf("Running test 6 to test realloc\n");

  printf("Creating a block of size 2048\n");
  char * ptr1 = ( char * ) malloc( 2048 );
  printf("pointer to the block: %p\n", ptr1);
  printf("reallocating the block\n");
  ptr1 = realloc(ptr1, 4096);
  printf("pointer must not change since its the last block\n");
  printf("After reallocting, pointer to the block: %p\n", ptr1);
  printf("------------------------------------------------------------------\n");
  printf("Creating another block of size 4096, after block 1\n");
  char * ptr2 = ( char * ) malloc( 4096 );
  printf("Freeing the new block\n");
  free( ptr2 );
  printf("Now I have free memory of 4096 after the first block\n");
  printf("Trying to reallocate a memory of 6154 to first block\n");
  ptr1 = realloc(ptr1, 6154);
  printf("Since there is available memory right after the current block\n");
  printf("After reallocation the pointer should remain same\n");
  printf("After reallocting, pointer to the block: %p\n", ptr1);
  printf("------------------------------------------------------------------\n");
  printf("Creating another block of size 2048, after block 1\n");
  ptr2 = ( char * ) malloc( 2048 );
  printf("Now there is no more memory after block 1\n");
  printf("Allocating more memory to block 1 will move its pointer\n");
  printf("Allocating 100000 memory to block 1\n");
  ptr1 = realloc(ptr1, 100000);
  printf("The pointer to block 1 should change\n");
  printf("The new pointer is: %p\n", ptr1);
  free(ptr1);
  free(ptr2);

  printf("------------------------------------------------------------------\n");
  int * my_ptr = ( int * ) malloc( 10 * sizeof(int) );
  printf("Making an Array and assigning values\n");
  int i;
  for (i = 0; i<10; i++)
  {
    my_ptr[i] = rand()%1000;
    printf("%d: %d\n", i+1, my_ptr[i]);
  }
  printf("Pointer to the array before realloc: %p\n", my_ptr);
  printf("------------------------------------------------------------------\n");
  printf("Reallocating space for the array: \n");
  my_ptr = realloc(my_ptr, 20 * sizeof(int));
  printf("Values must remain unchanged\n");
  for (i = 0; i<20; i++)
  {
    printf("%d: %d\n", i+1, i<10?my_ptr[i]:0);
  }
  printf("Pointer to the array after realloc: %p\n", my_ptr);
  printf("------------------------------------------------------------------\n");
  printf("Reallocating such that the pointer to array changes but data doesn't\n");
  int * temp = malloc(40);
  temp = temp;
  my_ptr = realloc(my_ptr, 25 * sizeof(int));
  printf("Values after realloc:\n");
  for (i = 0; i<25; i++)
  {
    printf("%d: %d\n", i+1, i<10?my_ptr[i]:0);
  }
  printf("Pointer to the array after realloc: %p\n", my_ptr);
  free(my_ptr);
  free(temp);
  return 0;
}
