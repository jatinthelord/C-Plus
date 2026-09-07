#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

int game(void *baseplate, void *player)
{
   struct baseplatePos{
      int *x;
      int *y;
      int *z;
   };

   struct baseplateL{
        int *l;
        int *w;
   };

   struct playerPos{
       int *x;
       int *y;
       int *z;
       int *w;
       int *l;
   };

}