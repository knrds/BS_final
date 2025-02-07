/******************************************************************************
 * FILE: echoall.c
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * First OSMP Executable, it simply echoes all arguments passed to it.
 ******************************************************************************/

#include <stdio.h>

int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    fprintf(stdout, "%s ", argv[i]);
  }
  fprintf(stdout, "\n");

  return 0;
}
