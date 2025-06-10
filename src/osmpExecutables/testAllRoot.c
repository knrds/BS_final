//
// Created by ChatGPT on 20.05.2025.
// Simuliert ein AllGather mit OSMP_Gather
//

#include <stdio.h>
#include <stdlib.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank;
    const int sendcount = 5;

    rv = OSMP_Init(&argc, &argv);
    if (rv == OSMP_FAILURE) return EXIT_FAILURE;

    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    rv = OSMP_Size(&size);
    if (rv == OSMP_FAILURE) {
        OSMP_Finalize();
        return EXIT_FAILURE;
    }


    int sendbuf[sendcount];
    for (int i = 0; i < sendcount; ++i)
        sendbuf[i] = rank + i;

    for (int root = 0; root < size; ++root) {
        int *recvbuf = NULL;

        if (rank == root) {
            recvbuf = malloc((size_t)size * (size_t)sendcount * sizeof(int));
            if (!recvbuf) {
                OSMP_Finalize();
                return EXIT_FAILURE;
            }

            for (int i = 0; i < size * sendcount; ++i)
                recvbuf[i] = -1;
        }


        rv = OSMP_Gather(sendbuf, sendcount, OSMP_INT,
                         recvbuf, sendcount, OSMP_INT,
                         root);
        if (rv == OSMP_FAILURE) {
            fprintf(stderr, "[%d] OSMP_Gather failed (root=%d)\n", rank, root);
            free(recvbuf);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }


        if (rank == root) {
            printf("\n[Root = %d] Gather-Ergebnis:\n", root);
            int errors = 0;
            for (int r = 0; r < size; ++r) {
                printf(" Rank %d: ", r);
                for (int i = 0; i < sendcount; ++i) {
                    int val = recvbuf[r * sendcount + i];
                    printf("%3d ", val);
                    if (val != r + i)
                        ++errors;
                }
                printf("\n");
            }
            if (errors == 0)
                printf("[OK] Gather bei Root %d erfolgreich.\n", root);
            else
                printf("[FEHLER] %d Werte fehlerhaft bei Root %d!\n", errors, root);
            free(recvbuf);
        }

    }

    OSMP_Finalize();
    return EXIT_SUCCESS;
}
