//
// Mehrprozess-Version: Jeder Prozess sendet seinen Rang an Root (Prozess 0)
// Root prüft, ob Inhalt == source
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../osmpLibrary/OSMP.h"

int main(int argc, char **argv) {
    int rv, size, rank, source, len;
    int message;

    rv = OSMP_Init(&argc, &argv);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "Fehler bei OSMP_Init\n");
        return EXIT_FAILURE;
    }

    rv = OSMP_Size(&size);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "Fehler bei OSMP_Size\n");
        return EXIT_FAILURE;
    }

    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "Fehler bei OSMP_Rank\n");
        return EXIT_FAILURE;
    }

    if (size < 2) {
        fprintf(stderr, "Dieses Beispiel benötigt mindestens 2 Prozesse.\n");
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        // Root: empfängt Nachrichten von allen anderen Prozessen
        for (int i = 1; i < size; i++) {
            int received;
            rv = OSMP_Recv(&received, 1, OSMP_INT, &source, &len);
            if (rv == OSMP_FAILURE) {
                fprintf(stderr, "Fehler bei OSMP_Recv\n");
                return EXIT_FAILURE;
            }

            printf("Root-Prozess %d hat %d Bytes von Prozess %d empfangen. Inhalt: \"%d\"\n",
                   rank, len, source, received);

            if (received == source) {
                printf("[OK] Nachricht stimmt mit Sender überein.\n");
            } else {
                printf("[FEHLER] Nachricht stimmt NICHT mit Sender überein.\n");
            }
        }

    } else {
        // Jeder andere Prozess sendet seinen Rang an den Root-Prozess
        message = rank;
        rv = OSMP_Send(&message, 1, OSMP_INT, 0);
        if (rv == OSMP_FAILURE) {
            fprintf(stderr, "Fehler bei OSMP_Send von Prozess %d\n", rank);
            return EXIT_FAILURE;
        }
        printf("Prozess %d hat seinen Rang an Root-Prozess 0 gesendet.\n", rank);
    }

    rv = OSMP_Finalize();
    if (rv == OSMP_FAILURE) {
        fprintf(stderr, "Fehler bei OSMP_Finalize\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
