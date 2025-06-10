/******************************************************************************
* FILE: osmpExecutable_SendIRecv.c
* DESCRIPTION:
* OSMP program with a simple pair of OSMP_Send/OSMP_Irecv calls
*
* LAST MODIFICATION: Darius Malysiak, March 21, 2023
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../osmpLibrary/OSMP.h"
#include "../osmpLibrary/osmpLib.h"


int main(int argc, char *argv[]) {
    int rv, size, rank, source, len, SIZE = 12;
    char *bufin, *bufout;
    OSMP_Request myrequest = NULL;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size != 2) {
        printf("This program requires exactly 2 processes\n");
        rv = OSMP_Finalize();
        return OSMP_FAILURE;
    }
    if (rank == 0) {
        // OSMP process 0
        bufin = calloc(1, (size_t) SIZE); // check for != NULL
        len = 12; // length
        memcpy(bufin, "Hello World", (size_t) len);
        sleep(5);
        printf("[SEND] Rank %d sending: %s\n", rank, bufin);
        rv = OSMP_Send(bufin, len, OSMP_BYTE, 1);
    } else {
        bufout = calloc(1, (size_t) SIZE); // check for != NULL
        rv = OSMP_CreateRequest(&myrequest);
        rv = OSMP_IRecv(bufout, SIZE, OSMP_BYTE, &source, &len, myrequest);

        do {
            int flag;
            rv = OSMP_Test(myrequest, &flag);
            if (rv == OSMP_FAILURE) {
                fprintf(stderr, "OSMP_Test failed\n");
                free(bufout);
                OSMP_Finalize();
                return OSMP_FAILURE;
            }

            if (flag) {
                // OSMP_IRecv() completed, use bufout
                printf("[RECV] Rank %d received: %s\n", rank, bufout);
            } else {
                printf("[RECV] Rank %d waiting for message...\n", rank);
                printf("[RECV] Rank %d doing business logic...\n", rank);
                // Warten bis die Nachricht empfangen wurde
                sleep(1); // 1 s warten
            }
        } while (len == 0); // Warten bis die Nachricht empfangen wurde

        printf("Should be Hello World: %s\n", bufout);

        rv = OSMP_RemoveRequest(&myrequest);
    }
    rv = OSMP_Finalize();
    UNUSED(rv);
    return 0;
}
