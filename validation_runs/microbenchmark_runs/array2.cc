#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#define ARRAY_SIZE 2000000

int main(int argc, char *argv[]) {
   uint64_t x;
   uint32_t array[ARRAY_SIZE];

   for (uint64_t i = 0; i < ARRAY_SIZE; i += 1024)
      array[i] = i;

   for (uint64_t i = 0; i < ARRAY_SIZE; i++)
      array[i] = i;

   printf("Which array element would you like to print?  ");
   scanf("%lu", &x);
   printf("array[%lu] = %u\n", x, array[x]);
   
   return(0);
}
