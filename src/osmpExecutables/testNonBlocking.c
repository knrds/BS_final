/**
 * @file nonblocking_test.c
 *
 * @brief Testprogramm für Nicht-Blockierendes Senden (ISend) und
 *        Empfangen (IRecv) ohne goto
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../osmpLibrary/OSMP.h"

#define MSG_SIZE 32

int main(int argc, char *argv[]) {
    int rv, size, rank, source = -1, len = 0;
    char sendbuf[MSG_SIZE], recvbuf[MSG_SIZE];
    OSMP_Request send_req = NULL, recv_req = NULL;
    int flag = 0;

    // 1. Init
    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    OSMP_Size(&size);
    OSMP_Rank(&rank);
    if (size != 2) {
        if (rank == 0)
            fprintf(stderr, "Dieser Test benötigt genau 2 Prozesse (aktuell %d)\n", size);
        OSMP_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        // --- Sender (Rank 0) ---
        snprintf(sendbuf, MSG_SIZE, "Hallo von Prozess %d", rank);

        if (OSMP_CreateRequest(&send_req) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] CreateRequest failed\n", rank);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        if (OSMP_ISend(sendbuf, (int)strlen(sendbuf) + 1, OSMP_BYTE, 1, send_req) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] ISend failed\n", rank);
            OSMP_RemoveRequest(&send_req);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        // Polling bis fertig
        do {
            if (OSMP_Test(send_req, &flag) != OSMP_SUCCESS) {
                fprintf(stderr, "[%d] Test failed\n", rank);
                OSMP_RemoveRequest(&send_req);
                OSMP_Finalize();
                return EXIT_FAILURE;
            }
            if (!flag) {
                printf("[%d] ISend noch nicht abgeschlossen, warte...\n", rank);
                sleep(1); // Simuliere Arbeit
            }
        } while (!flag);

        printf("[%d] ISend abgeschlossen, gesendete Nachricht: \"%s\"\n",
               rank, sendbuf);

        OSMP_RemoveRequest(&send_req);

    } else {
        // --- Empfänger (Rank 1) ---
        if (OSMP_CreateRequest(&recv_req) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] CreateRequest failed\n", rank);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        if (OSMP_IRecv(recvbuf, MSG_SIZE, OSMP_BYTE, &source, &len, recv_req) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] IRecv failed\n", rank);
            OSMP_RemoveRequest(&recv_req);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        printf("[%d] Simuliere interne Logik...\n", rank);
        sleep(1); // Simuliere Arbeit

        if (OSMP_Test(recv_req, &flag) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] Test failed\n", rank);
            OSMP_RemoveRequest(&recv_req);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }
        if (!flag) {
            printf("[%d] Nachricht noch nicht da, gehe in Wait()\n", rank);
        }

        if (OSMP_Wait(recv_req) != OSMP_SUCCESS) {
            fprintf(stderr, "[%d] Wait failed\n", rank);
            OSMP_RemoveRequest(&recv_req);
            OSMP_Finalize();
            return EXIT_FAILURE;
        }

        printf("[%d] IRecv abgeschlossen von Rank %d, Länge %d, Inhalt: \"%s\"\n",
               rank, source, len, recvbuf);

        OSMP_RemoveRequest(&recv_req);
    }

    // Finalize
    rv = OSMP_Finalize();
    printf("[%d] OSMP_Finalize returned %d\n", rank, rv);
    return (rv == OSMP_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
}
