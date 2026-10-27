#include <stdio.h>
#include <stdlib.h>

int main() {
  // Allocate memory
  int *ptr;
  ptr = calloc(4, sizeof(*ptr));

  // Write to the memory
  *ptr = 2;
  ptr[1] = 4;
  ptr[2] = 6;

  // Read from the memory
  printf("%d\n", *ptr);
  printf("%d %d %d %d",ptr[0], ptr[1], ptr[2], ptr[3]);

  return 0;
}