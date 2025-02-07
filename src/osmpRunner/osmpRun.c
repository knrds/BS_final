/******************************************************************************
 * FILE: osmpRun.c
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * Diese Datei enthält die main Funktion für den OSMP-Runner.
 ******************************************************************************/

#include "osmpRun.h"

int main(int argc, char **argv) {
  pid_t pid;
  int a;

  fprintf(stdout,
          "Dies ist eine rudimentäre Implementierung des Runners und dient "
          "dazu Ihnen zu zeigen, wie es prinzipiell ablaufen sollte. Ihre "
          "Aufgabe ist es, diese Implementierung (den Code) entsprechend der "
          "Aufgabenstellung zu ändern.\n");

  // TODO: Implementieren Sie hier die Funktionalität des Runners und passen Sie
  // den Code entsprechend an. Das bedeutet, dass Sie hier entsprechende
  // Code-Blöcke löschen, neu schreiben oder anpassen müssen.

  if (2 > argc) {
    fprintf(stderr, "Usage: %s <executable> [args...]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (-1 == pid) {
    fprintf(stderr, "Fehler bei fork\n");
    exit(EXIT_FAILURE);
  }

  if (0 == pid) {
    // Kindprozess

    // Ändern Sie gerne die exec* Funktion, wenn Sie eine andere benutzen
    // möchten.
    execv(argv[1], &argv[1]);
    // Execv ist fehlgeschlagen...
    perror("execv");
    exit(EXIT_FAILURE);
  }

  // Elternprozess
  a = OSMP_SUCCESS;
  if (OSMP_FAILURE == a) {
    fprintf(stderr, "Fehler bei ...\n");
    exit(EXIT_FAILURE);
  }
  printf("OSMP_SUCCESS: %d\n", a);
  a = OSMP_FAILURE;
  if (OSMP_FAILURE == a) {
    fprintf(stderr, "Fehler bei ...\n");
    exit(EXIT_FAILURE);
  }

  return 0;
}
