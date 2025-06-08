#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../osmpLibrary/OSMP.h"
/**
 * Jeder Prozess sendet nicht-blockierend Nachrichte an jeden Prozess und empfängt nachrichten von jedem anderem
 * @param argc Unused
 * @param argv Unused
 * @return OSMP_SUCCESS bei Erfolg, sonst OSMP_Failure
 */
int main(int argc, char *argv[]) {
    int returnVal, totalProcesses, currentProcess;
    int inputBuffer, messageLength;
    returnVal = OSMP_Init(&argc, &argv);
    returnVal = OSMP_Size(&totalProcesses);
    returnVal = OSMP_Rank(&currentProcess);

    OSMP_Request sendRequests[totalProcesses - 1];
    int requestIndex = 0;
    for (int i = 0; i < totalProcesses; i++) {
        if (i != currentProcess) {
            returnVal = OSMP_CreateRequest(&sendRequests[requestIndex]);
            if (returnVal == OSMP_FAILURE) {
                printf("Failed creating send request nr. %d\n", requestIndex);
            } else {
                inputBuffer = currentProcess;
                returnVal = OSMP_ISend(&inputBuffer, 1, OSMP_INT, i, sendRequests[requestIndex]);

                if (returnVal != OSMP_SUCCESS) {
                    printf("Failed Sending\n");
                }
            }
            requestIndex += 1;
        }
    }

    for (int i = 0; i < totalProcesses - 1; i++) {
        returnVal = OSMP_Wait(sendRequests[i]);
        if (returnVal == OSMP_FAILURE) {
            printf("Failed waiting for send request: %d\n", i);
        }
        returnVal = OSMP_RemoveRequest(&sendRequests[i]);
        if (returnVal == OSMP_FAILURE) {
            printf("Couldn't free send Request %d\n", i);
        }
    }

    OSMP_Request recvRequests[totalProcesses - 1];
    int outputBuffer[totalProcesses - 1];
    int sources[totalProcesses - 1];
    for (int i = 0; i < totalProcesses - 1; i++) {

        returnVal = OSMP_CreateRequest(&recvRequests[i]);

        if (returnVal == OSMP_FAILURE) {
            printf("Creating recv request %d failed\n", i);
        } else {
            // OSMP process 1
            returnVal = OSMP_IRecv(&outputBuffer[i], 1, OSMP_INT, &sources[i], &messageLength, recvRequests[i]);

            if (returnVal != OSMP_SUCCESS) {
                printf("Error receiving\n");
            }
        }

    }

    for (int i = 0; i < totalProcesses - 1; i++) {
        returnVal = OSMP_Wait(recvRequests[i]);

        if (returnVal == OSMP_FAILURE) {
            printf("Failed waiting for request: %d\n", i);
            continue;
        }
        OSMP_RemoveRequest(&recvRequests[i]);

        if (sources[i] == outputBuffer[i]) {
            printf("Process %d successfully received a message from %d\n", currentProcess, sources[i]);
        } else {
            printf("Process %d failed to receive message from %d\n", currentProcess, sources[i]);
        }
    }

    returnVal = OSMP_Finalize();
    printf("%d\n", returnVal);
    return 0;
}
