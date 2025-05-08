//
// Created by konrad_laptop on 07.05.25.
//
// Test: Blockierverhalten der Sender wenn Empfänger verzögert
// Ein Empfängerprozess schläft lange, während Sender Nachrichten versenden

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../osmpLibrary/OSMP.h"

#define NUM_MESSAGES 5
#define MAX_MSG_LEN 128
#define RECEIVER_RANK 0

int main(int argc, char **argv) {
    int rank, size, rv;
    char msg[MAX_MSG_LEN];
    char buf[MAX_MSG_LEN];
    int source, len;

    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "[ERROR] OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    OSMP_Rank(&rank);
    OSMP_Size(&size);

    if (rank != RECEIVER_RANK) {
        // Senderprozesse senden alle Nachrichten an den Empfänger (Rank 0)
        for (int i = 0; i < NUM_MESSAGES; i++) {
            snprintf(msg, MAX_MSG_LEN, "[Send %d] Hello from %d", i, rank);
            int msg_len = (int)(strlen(msg) + 1);

            rv = OSMP_Send(msg, msg_len, OSMP_UNSIGNED_CHAR, RECEIVER_RANK);
            if (rv != OSMP_SUCCESS) {
                fprintf(stderr, "[ERROR] Rank %d Send %d failed\n", rank, i);
            } else {
                printf("[SEND] Rank %d → %d: %s\n", rank, RECEIVER_RANK, msg);
            }
        }
    } else {
        // Empfängerprozess wartet absichtlich, damit Sender blockieren
        printf("[RECV] Rank 0 sleeping for 5 seconds...\n");
        sleep(5);

        for (int i = 0; i < NUM_MESSAGES * (size - 1); i++) {
            rv = OSMP_Recv(buf, MAX_MSG_LEN, OSMP_UNSIGNED_CHAR, &source, &len);
            if (rv != OSMP_SUCCESS) {
                fprintf(stderr, "[ERROR] Recv %d failed at rank 0\n", i);
            } else {
                char expected[MAX_MSG_LEN];
                int local_index = i % NUM_MESSAGES; // Annäherung – genauer wäre Sender-Tracking
                snprintf(expected, MAX_MSG_LEN, "[Send %d] Hello from %d", local_index, source);

                printf("[RECV] Rank 0 got %d bytes from %d: %s\n", len, source, buf);
                if (strcmp(buf, expected) == 0) {
                    printf("[OK] Nachricht von %d korrekt: %s\n", source, buf);
                } else {
                    printf("[FEHLER] Nachricht von %d unerwartet! Erwartet: %s\n", source, expected);
                }
            }
        }
    }

    OSMP_Finalize();
    return 0;
}
