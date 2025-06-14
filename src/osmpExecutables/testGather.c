//
// Created by ChatGPT on 20.05.2025.
// Testprogramm für OSMP_Gather
//
#include <stdio.h>
#include <stdlib.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank;
    const int root = 0;
    const int sendcount = 10;

    rv = OSMP_Init(&argc, &argv);
    if (rv == OSMP_FAILURE) return EXIT_FAILURE;

    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    // Jeder füllt seinen Puffer mit rank*100 + i
    int sendbuf[sendcount];
    for (int i = 0; i < sendcount; ++i)
        sendbuf[i] = rank + i;

    // Root reserviert recvbuf
    int *recvbuf = NULL;
    if (rank == root) {
        rv = OSMP_Size(&size);
        if (rv == OSMP_FAILURE) {
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        recvbuf = malloc(
            (size_t) size
            * (size_t) sendcount
            * sizeof(int)
        );
        if (!recvbuf) {
            OSMP_Finalize();
            return EXIT_FAILURE;
        }
        // Optional: init auf -1
        for (int i = 0; i < size * sendcount; ++i) recvbuf[i] = -1;
    }


    // Gather aufrufen
    rv = OSMP_Gather(sendbuf, sendcount, OSMP_INT,
                     recvbuf, sendcount, OSMP_INT,
                     root);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "OSMP_Gather failed on rank %d\n", rank);
        free(recvbuf);
        OSMP_Finalize();
        return EXIT_FAILURE;
    }


    // Nur Root validiert und gibt aus
    if (rank == root) {
        printf("Gather-Ergebnis (root=%d):\n", root);
        int errors = 0;
        // Über alle Prozesse itterieren
        for (int r = 0; r < size; ++r) {
            printf(" Rank %d: ", r);
            //Über sendcount itterieren
            for (int i = 0; i < sendcount; ++i) {
                //ReceiveBuffer an stelle r*sendcount+i
                //recvbuf[1 * 10 + 3] = recvbuf[13]
                // 4. Wert (i = 3) von Rank 1 (r = 1)
                int got = recvbuf[r * sendcount + i];
                printf("%3d ", got);
                if (got != r + i) ++errors;
            }
            printf("\n");
        }
        if (errors == 0)
            printf("[OK] Daten aller Prozesse korrekt gesammelt.\n");
        else
            printf("[FEHLER] %d Werte stimmen nicht!\n", errors);
        free(recvbuf);
    }

    OSMP_Finalize();
    return EXIT_SUCCESS;
}
