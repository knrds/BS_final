//
// Test: Race Condition Simulation mit Inhaltsprüfung
// Jeder Prozess sendet parallel 5 Nachrichten an den nächsten im Ring
// und prüft, ob die empfangenen Nachrichten korrekt formatiert sind.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../osmpLibrary/OSMP.h"

#define MAX_MSG_LEN 128
#define NUM_SENDS 5
#define NUM_RECVS 5

void *send_loop(void *arg) {
    (void)arg;
    int rank, size;
    OSMP_Rank(&rank);
    OSMP_Size(&size);

    for (int i = 0; i < NUM_SENDS; i++) {
        char msg[MAX_MSG_LEN];
        snprintf(msg, MAX_MSG_LEN, "[SEND %d] from %d", i, rank);
        int len = (int)(strlen(msg) + 1);
        int dest = (rank + 1) % size;

        if (OSMP_Send(msg, len, OSMP_UNSIGNED_CHAR, dest) != OSMP_SUCCESS) {
            fprintf(stderr, "[ERROR] Send %d from %d failed\n", i, rank);
        } else {
            printf("[SEND] %d → %d: %s\n", rank, dest, msg);
        }
    }
    return NULL;
}

void *recv_loop(void *arg) {
    (void)arg;
    int rank, source, len;
    char buf[MAX_MSG_LEN];
    OSMP_Rank(&rank);

    for (int i = 0; i < NUM_RECVS; i++) {
        if (OSMP_Recv(buf, MAX_MSG_LEN, OSMP_UNSIGNED_CHAR, &source, &len) == OSMP_SUCCESS) {
            printf("[RECV] %d <- %d: %s\n", rank, source, buf);

            // Erwartete Nachricht erzeugen
            char expected[MAX_MSG_LEN];
            snprintf(expected, MAX_MSG_LEN, "[SEND %d] from %d", i, source);

            if (strcmp(buf, expected) == 0) {
                printf("[OK] Nachricht korrekt empfangen\n");
            } else {
                printf("[FEHLER] Nachricht unerwartet! Erwartet: %s\n", expected);
            }

        } else {
            fprintf(stderr, "[ERROR] Recv %d at rank %d failed\n", i, rank);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (OSMP_Init(&argc, &argv) != OSMP_SUCCESS) {
        fprintf(stderr, "[ERROR] Init failed\n");
        return EXIT_FAILURE;
    }

    pthread_t sender, receiver;
    pthread_create(&sender, NULL, send_loop, NULL);
    pthread_create(&receiver, NULL, recv_loop, NULL);


    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    OSMP_Finalize();
    return 0;
}
