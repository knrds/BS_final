//
// Created by erik on 03.05.25.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h> // für memcmp
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank, source, len;
    char bufin[1024], bufout[1024];

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

    // Nachricht füllen mit "ABCDE...ABC..."
    for (int i = 0; i < 1023; i++) {
        bufin[i] = (char) ('A' + i % 26);
    }
    bufin[1023] = '\0';

    if (0 == rank) {
        // Sender
        rv = OSMP_Send(bufin, 1024, OSMP_UNSIGNED_CHAR, 1);
        if (OSMP_FAILURE == rv) {
            fprintf(stderr, "OSMP_Send failed\n");
            return EXIT_FAILURE;
        }

    } else {
        // Empfänger
        rv = OSMP_Recv(bufout, 1024, OSMP_UNSIGNED_CHAR, &source, &len);
        if (OSMP_FAILURE == rv) {
            return EXIT_FAILURE;
        }
        printf("OSMP process %d received %d bytes from %d\n", rank, len, source);

        // Vergleich mit erwartetem Inhalt
        if (memcmp(bufin, bufout, 1024) == 0) {
            printf("[OK] Empfangene Nachricht stimmt mit gesendeter überein.\n");
        } else {
            printf("[FEHLER] Nachricht wurde verändert oder nicht korrekt empfangen!\n");
        }
    }

    rv = OSMP_Finalize();
    if (OSMP_FAILURE == rv) {
        return EXIT_FAILURE;
    }

    return 0;
}
