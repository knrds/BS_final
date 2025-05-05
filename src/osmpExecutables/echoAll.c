//
// Created by knrd on 02.04.25.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char *argv[]) {
    int rv;
    int size, rank;

    // Initialisiere OSMP
    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    // Bestimme Gesamtanzahl der Prozesse
    rv = OSMP_Size(&size);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "OSMP_Size failed\n");
        return EXIT_FAILURE;
    }

    // Bestimme eigenen Rank
    rv = OSMP_Rank(&rank);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "OSMP_Rank failed\n");
        return EXIT_FAILURE;
    }

    // Ausgabe
    fprintf(stdout, "Programmname: %s\n", argv[0]);
    for (int i = 1; i < argc ; i++) {
        fprintf(stdout, "Argument %d: %s\n", i, argv[i]);
    }

    fprintf(stdout, "PID: %d | PPID: %d\n", getpid(), getppid());

    fprintf(stdout, "Hallo von Rank %d von %d\n", rank, size);

    // Finalize
    rv = OSMP_Finalize();
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "OSMP_Finalize failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
