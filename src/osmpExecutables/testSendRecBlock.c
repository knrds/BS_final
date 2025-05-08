//
// Test: Blockierverhalten der Sender wenn Empfänger verzögert
// Jeder Sender sendet NUM_MESSAGES an Empfängerprozess 0
// Der Empfänger prüft, ob alle erwarteten Nachrichten korrekt empfangen wurden
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../osmpLibrary/OSMP.h"

#define NUM_MESSAGES 5
#define MAX_MSG_LEN 128
#define RECEIVER_RANK 0
#define MAX_PROCESSES 64  // vorsichtshalber, falls OSMP_MAX_PROCESSES nicht definiert

int main(int argc, char **argv) {
    int rank, size, rv;
    char msg[MAX_MSG_LEN];
    char buf[MAX_MSG_LEN];
    int source, len;

    int received_flags[MAX_PROCESSES][NUM_MESSAGES] = {{0}};

    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "[ERROR] OSMP_Init failed\n");
        return EXIT_FAILURE;
    }

    OSMP_Rank(&rank);
    OSMP_Size(&size);

    if (rank != RECEIVER_RANK) {
        // Senderprozesse senden NUM_MESSAGES an Empfänger
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
        // Empfänger wartet, damit Sender ggf. blockieren
        printf("[RECV] Rank 0 sleeping for 5 seconds...\n");
        sleep(5);

        for (int i = 0; i < NUM_MESSAGES * (size - 1); i++) {
            rv = OSMP_Recv(buf, MAX_MSG_LEN, OSMP_UNSIGNED_CHAR, &source, &len);
            if (rv != OSMP_SUCCESS) {
                fprintf(stderr, "[ERROR] Recv %d failed at rank 0\n", i);
                continue;
            }

            printf("[RECV] Rank 0 got %d bytes from %d: %s\n", len, source, buf);

            int parsed_idx = -1, parsed_sender = -1;
            if (sscanf(buf, "[Send %d] Hello from %d", &parsed_idx, &parsed_sender) == 2) {
                if (parsed_sender != source) {
                    printf("[FEHLER] Quelle stimmt nicht! Erwartet %d, gelesen %d\n", source, parsed_sender);
                } else if (parsed_idx < 0 || parsed_idx >= NUM_MESSAGES) {
                    printf("[FEHLER] Ungültiger Nachrichtenindex %d von %d\n", parsed_idx, source);
                } else if (received_flags[source][parsed_idx]) {
                    printf("[FEHLER] Doppelte Nachricht %d von %d!\n", parsed_idx, source);
                } else {
                    received_flags[source][parsed_idx] = 1;
                    printf("[OK] Nachricht %d von %d korrekt: %s\n", parsed_idx, source, buf);
                }
            } else {
                printf("[FEHLER] Ungültiges Nachrichtenformat: %s\n", buf);
            }
        }
    }

    OSMP_Finalize();
    return 0;
}
