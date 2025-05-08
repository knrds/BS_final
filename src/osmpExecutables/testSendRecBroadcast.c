//
// Created by konrad_laptop on 07.05.25.
//
// Test: Broadcast von Rang 0 an alle anderen Prozesse
// Jeder Empfänger gibt die Nachricht aus, die er erhalten hat und prüft den Inhalt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../osmpLibrary/OSMP.h"

#define MAX_MSG_LEN 128

int main(int argc, char **argv) {
    int rank, size, rv;
    char msg[MAX_MSG_LEN];
    char recv_buf[MAX_MSG_LEN];
    int source, len;

    rv = OSMP_Init(&argc, &argv);
    if (rv != OSMP_SUCCESS) {
        fprintf(stderr, "[ERROR] OSMP_Init failed\n");
        return EXIT_FAILURE;
    }


    OSMP_Rank(&rank);
    OSMP_Size(&size);

    snprintf(msg, MAX_MSG_LEN, "[Broadcast] Hello from process 0");

    if (rank == 0) {
        // Rang 0 sendet dieselbe Nachricht an alle anderen Prozesse
        for (int dest = 1; dest < size; dest++) {
            rv = OSMP_Send(msg, (int)(strlen(msg) + 1), OSMP_UNSIGNED_CHAR, dest);
            if (rv != OSMP_SUCCESS) {
                fprintf(stderr, "[ERROR] Send to %d failed\n", dest);
            } else {
                printf("[SEND] Rank 0 → %d: %s\n", dest, msg);
            }
        }
    } else {
        // Alle anderen Prozesse empfangen je eine Nachricht
        rv = OSMP_Recv(recv_buf, MAX_MSG_LEN, OSMP_UNSIGNED_CHAR, &source, &len);
        if (rv != OSMP_SUCCESS) {
            fprintf(stderr, "[ERROR] Recv at rank %d failed\n", rank);
        } else {
            printf("[RECV] Rank %d got %d bytes from %d: %s\n", rank, len, source, recv_buf);
            if (strcmp(recv_buf, msg) == 0) {
                printf("[OK] Rank %d received correct broadcast message.\n", rank);
            } else {
                printf("[FEHLER] Rank %d received wrong message!\n", rank);
            }
        }
    }

    OSMP_Finalize();
    return 0;
}
