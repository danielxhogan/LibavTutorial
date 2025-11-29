#include <stdio.h>
#include "libavtranscode/avtranscode.h"

int main(int argc, char **argv)
{
  if (argc != 6) {
    printf("avtranscode takes 5 arguments.\n");
    return 0;
  }

  avtranscode(argv[1], argv[2], argv[3], argv[4], argv[5]);

  return 0;
}