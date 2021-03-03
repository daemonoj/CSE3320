#include <stdlib.h>
#include <stdio.h>

int main()
{
  printf("Running test 5 to test calloc\n");

  char * ptr1 = ( char * ) calloc( 10, sizeof(char) );
  int i;
  for (i = 0; i<10; i++)
  {
  	printf("%d: %c\n", i+1, ptr1[i]);
  }

  free( ptr1 );

  int * ptr2 = ( int * ) calloc( 10, sizeof(int) );
  for (i = 0; i<10; i++)
  {
  	printf("%d: %d\n", i+1, ptr2[i]);
  }
  free( ptr2 );

  return 0;
}
