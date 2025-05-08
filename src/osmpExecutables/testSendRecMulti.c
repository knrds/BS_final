//
// Created by konrad_laptop on 07.05.25.
//
// Testprogramm für mehrere Sender und Empfänger
// Jeder Prozess sendet mehrere Nachrichten an den nächsten Prozess im Ring
// und empfängt dieselbe Anzahl von seinem Vorgänger
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../osmpLibrary/OSMP.h"

#define NUM_MESSAGES 5
#define MAX_MSG_LEN 128

int main(int argc, char **argv) {
    int rank, size, rv;
    char send_buf[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    int source, len;

    // Initialisiere OSMP-Umgebung
    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "[ERROR] OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    OSMP_Rank(&rank);
    OSMP_Size(&size);

    // Ziel: sende an nächsten Prozess im Ring
    int target = (rank + 1) % size;
    int expected_sender = (rank - 1 + size) % size;

    // Senden von NUM_MESSAGES Nachrichten an Zielprozess
    for (int i = 0; i < NUM_MESSAGES; i++) {
        snprintf(send_buf, MAX_MSG_LEN, "[msg %d] Hello from %d to %d", i, rank, target);
        int msg_len = (int)(strlen(send_buf) + 1);  // sicherer Cast von size_t zu int

        rv = OSMP_Send(send_buf, msg_len, OSMP_UNSIGNED_CHAR, target);
        if (rv != OSMP_SUCCESS) {
            fprintf(stderr, "[ERROR] Send %d failed at rank %d\n", i+1, rank);
        } else {
            printf("[SEND] Rank %d → %d: %s\n", rank, target, send_buf);
        }
    }

    // Empfang von NUM_MESSAGES Nachrichten
    for (int i = 0; i < NUM_MESSAGES; i++) {
        rv = OSMP_Recv(recv_buf, MAX_MSG_LEN, OSMP_UNSIGNED_CHAR, &source, &len);
        if (rv != OSMP_SUCCESS) {
            fprintf(stderr, "[ERROR] Recv %d failed at rank %d\n", i, rank);
        } else {
            char expected[MAX_MSG_LEN];
            snprintf(expected, MAX_MSG_LEN, "[msg %d] Hello from %d to %d", i, expected_sender, rank);

            printf("[RECV] Rank %d got %d bytes from %d: %s\n", rank, len, source, recv_buf);
            if (strcmp(recv_buf, expected) == 0) {
                printf("[OK] Nachricht korrekt empfangen.\n");
            } else {
                printf("[FEHLER] Nachricht falsch! Erwartet: %s\n", expected);
            }
        }
    }

    OSMP_Finalize();
    return 0;
}
