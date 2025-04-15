/******************************************************************************
 * FILE: osmpLib.c
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * In dieser Quelltext-Datei sind Implementierungen der OSMP Bibliothek zu
 * finden.
 ******************************************************************************/

#include "osmpLib.h"
#include <stdlib.h>

int OSMP_GetMaxPayloadLength(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_MAX_PAYLOAD_LENGTH;
}

int OSMP_GetMaxSlots(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_MAX_SLOTS;
}

int OSMP_GetMaxMessagesProc(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_MAX_MESSAGES_PROC;
}

int OSMP_GetFailure(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_GetSucess(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_SUCCESS;
}

int OSMP_Init(const int *argc, char ***argv) {

    if (OSMP_FAILURE == OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_Init() called")) {
        return OSMP_FAILURE;
    }

    if (argc == NULL || argv == NULL) {
        return OSMP_FAILURE;
    }



  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_SUCCESS;
}

int OSMP_SizeOf(OSMP_Datatype datatype, unsigned int *size) {
    switch (datatype) {
        case OSMP_SHORT:
            *size = sizeof(short);
            break;
        case OSMP_INT:
            *size = sizeof(int);
            break;
        case OSMP_LONG:
            *size = sizeof(long);
            break;
        case OSMP_UNSIGNED_CHAR:
            *size = sizeof(unsigned char);
            break;
        case OSMP_UNSIGNED:
            *size = sizeof(unsigned);
            break;
        case OSMP_UNSIGNED_SHORT:
            *size = sizeof(unsigned short);
            break;
        case OSMP_UNSIGNED_LONG:
            *size = sizeof(unsigned long);
            break;
        case OSMP_FLOAT:
            *size = sizeof(float);
            break;
        case OSMP_DOUBLE:
            *size = sizeof(double);
            break;
        case OSMP_BYTE:
            *size = sizeof(char);
            break;
        default:
            return OSMP_FAILURE;
    }
    return OSMP_SUCCESS;
}

int OSMP_Size(int *size) {
  UNUSED(size);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Rank(int *rank) {
  UNUSED(rank);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest) {
  UNUSED(buf);
  UNUSED(count);
  UNUSED(datatype);
  UNUSED(dest);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source,
              int *len) {
  UNUSED(buf);
  UNUSED(count);
  UNUSED(datatype);
  UNUSED(source);
  UNUSED(len);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Finalize(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Barrier(void) {
  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Gather(void *sendbuf, int sendcount, OSMP_Datatype sendtype,
                void *recvbuf, int recvcount, OSMP_Datatype recvtype,
                int root) {
  UNUSED(sendbuf);
  UNUSED(sendcount);
  UNUSED(sendtype);
  UNUSED(recvbuf);
  UNUSED(recvcount);
  UNUSED(recvtype);
  UNUSED(root);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_ISend(const void *buf, int count, OSMP_Datatype datatype, int dest,
               OSMP_Request request) {
  UNUSED(buf);
  UNUSED(count);
  UNUSED(datatype);
  UNUSED(dest);
  UNUSED(request);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_IRecv(void *buf, int count, OSMP_Datatype datatype, int *source,
               int *len, OSMP_Request request) {
  UNUSED(buf);
  UNUSED(count);
  UNUSED(datatype);
  UNUSED(source);
  UNUSED(len);
  UNUSED(request);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Test(OSMP_Request request, int *flag) {
  UNUSED(request);
  UNUSED(flag);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Wait(OSMP_Request request) {
  UNUSED(request);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_CreateRequest(OSMP_Request *request) {
  UNUSED(request);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_RemoveRequest(OSMP_Request *request) {
  UNUSED(request);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}

int OSMP_Log(OSMP_Verbosity verbosity, char *message) {
  UNUSED(verbosity);
  UNUSED(message);

  // TODO: Implementieren Sie hier die Funktionalität der Funktion.
  return OSMP_FAILURE;
}
