//
// testGatherAlphabet.c
// Testprogramm für OSMP_Gather mit dem Alphabet
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank;
    const int root = 0;

    rv = OSMP_Init(&argc, &argv);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    rv = OSMP_Size(&size);
    if (rv == OSMP_FAILURE) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }


    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    // Gesamtlänge des Alphabets
    const int N = 26;
    // Jeder Prozess sendet genau sendcount = ceil(26/size) Zeichen,
    // der letzte Puffer wird ggf. mit '\0' aufgefüllt.
    int block = (N + size - 1) / size;  // teilt das Alphabet auf, indem es durch die Anzahl der Prozesse geteilt wird
    char *sendbuf = malloc((size_t)block);
    if (!sendbuf) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    // Fülle sendbuf mit dem entsprechenden Teil des Alphabets
    // A=65 ... Z=90
    for (int i = 0; i < block; ++i) {
        int idx = rank * block + i;
        if (idx < N)
            sendbuf[i] = (char)('A' + idx);
        else
            sendbuf[i] = '\0'; // Padding
    }

    // Root reserviert den Empfangspuffer: size*block Zeichen (+ Null-Term)
    char *recvbuf = NULL;
    if (rank == root) {
        recvbuf = calloc((size_t)size * (size_t)block + 1, 1);
        if (!recvbuf) {
            free(sendbuf);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }
    }

    // Gather
    rv = OSMP_Gather(sendbuf, block, OSMP_BYTE,
                     recvbuf, block, OSMP_BYTE,
                     root);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "OSMP_Gather failed on rank %d\n", rank);
        free(sendbuf);
        free(recvbuf);
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    // Root druckt das rekonstruierte Alphabet (bis Null-Term)
    if (rank == root) {
        printf("Rekonstruiertes Alphabet: \"");
        for (int i = 0; i < size * block; ++i) {
            if (recvbuf[i] == '\0') break;
            putchar(recvbuf[i]);
        }
        printf("\"\n");
    }

    free(sendbuf);
    free(recvbuf);
    OSMP_Finalize();
    return EXIT_SUCCESS;
}
