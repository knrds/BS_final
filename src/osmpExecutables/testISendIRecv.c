/**
* @file OSMP Executable für Testen
 *
 * @brief Sendet und empfängt Nachrichten mit ISend und IRecv
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../osmpLibrary/OSMP.h"

/**
 * Nicht-Blockierendes Senden in Kombination mit Nicht-Blockierendem Empfangen
 * @param argc Unused
 * @param argv Unused
 * @return OSMP_SUCCESS bei Erfolg, sonst OSMP_FAILURE
 */
int main(int argc, char *argv[]) {
    int rv, size, rank, source, len, SIZE = 12;
    char *bufin, *bufout;
    OSMP_Request myrequest = NULL, myrequest2 = NULL;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size != 2) {
        /* Fehlerbehandlung */
    }
    if (rank == 0) {
        // OSMP process 0
        bufin = malloc((size_t) SIZE); // check for != NULL
        len = 12; // length
        memcpy(bufin, "Hello World", (size_t) len);
        rv = OSMP_CreateRequest(&myrequest);
        rv = OSMP_ISend(bufin, len, OSMP_BYTE, 1, myrequest);
        rv = OSMP_Wait(myrequest);
        rv = OSMP_RemoveRequest(&myrequest);
    } else {
        // OSMP process 1
        bufout = malloc((size_t) SIZE); // check for != NULL
        rv = OSMP_CreateRequest(&myrequest2);
        rv = OSMP_IRecv(bufout, SIZE, OSMP_BYTE, &source, &len, myrequest2);
        // do something important…
        // check if operation is completed and wait if not
        rv = OSMP_Wait(myrequest2);
        // OSMP_IRecv() completed, use bufout
        printf("%s\n", bufout);
        OSMP_RemoveRequest(&myrequest);
    }
    rv = OSMP_Finalize();
    printf("%d\n", rv);
    return 0;
}
