//
// Created by erik on 03.05.25.
//
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank;

    rv = OSMP_Init(&argc, &argv);
    if (OSMP_FAILURE == rv) {
        return EXIT_FAILURE;
    }

    rv = OSMP_Size(&size);
    if (OSMP_FAILURE == rv) {
        return EXIT_FAILURE;
    }

    rv = OSMP_Rank(&rank);
    if (OSMP_FAILURE == rv) {
        return EXIT_FAILURE;
    }

    if (2 != size) {
        // Falsche Anzahl an Prozessen, diese Implementierung ist nur für 2 Prozesse gedacht
        return EXIT_FAILURE;
    }

    sleep(rank == 0 ? 1 : 5);

    fprintf(stderr, "OSMP process %d is waiting at the barrier\n", rank);

    rv = OSMP_Barrier();
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "OSMP_Barrier failed in process %d\n", rank);
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    fprintf(stderr, "OSMP process %d passed the barrier\n", rank);

    rv = OSMP_Finalize();
    if (OSMP_FAILURE == rv) {
        return EXIT_FAILURE;
    }

    return 0;
}
