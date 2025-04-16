/******************************************************************************
 * FILE: osmpLib.c
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * In dieser Quelltext-Datei sind Implementierungen der OSMP Bibliothek zu
 * finden.
 ******************************************************************************/

#include "osmpLib.h"
#include "OSMP.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static osmp_shared_info_t *osmp_shared = NULL;
static int osmp_rank = -1;

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
    UNUSED(argc);
    UNUSED(argv);

    // Erstelle den Shared Memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return OSMP_FAILURE;
    }

    // Setze die Größe des Shared Memory
    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        return OSMP_FAILURE;
    }

    // Initialisiere den Shared Memory
    osmp_shared = (osmp_shared_info_t *)ptr;

    // Setze die Anzahl der Prozesse auf 0
    pid_t my_pid = getpid();
    osmp_rank = -1;

    // Durchsuche pid_map auf meine PID
    for (int i = 0; i < OSMP_MAX_PROCESSES; i++) {
        if (osmp_shared->pid_map[i] == my_pid) {
            osmp_rank = i;
            break;
        }
    }

    if (osmp_rank == -1) {
        fprintf(stderr, "OSMP_Init: PID %d not found in pid_map\n", my_pid);
        return OSMP_FAILURE;
    }

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
    // Überprüfen Sie, ob die Shared Memory-Struktur initialisiert ist
    if (!osmp_shared || !size) {
        fprintf(stderr, "OSMP_Size: Invalid parameters\n");
        return OSMP_FAILURE;
    }

    // Überprüfen Sie, ob der Prozess initialisiert ist
    *size = osmp_shared->process_count;
    return OSMP_SUCCESS;
}


int OSMP_Rank(int *rank) {
    // Überprüfen Sie, ob die Shared Memory-Struktur initialisiert ist
    if (!osmp_shared || !rank) {
        fprintf(stderr, "OSMP_Rank: Invalid parameters\n");
        return OSMP_FAILURE;
    }

    // Überprüfen Sie, ob der Prozess initialisiert ist
    if (osmp_rank == -1) {
        fprintf(stderr, "OSMP_Rank: Not initialized or rank unknown\n");
        return OSMP_FAILURE;
    }

    // Geben Sie den Rang des Prozesses zurück
    *rank = osmp_rank;
    return OSMP_SUCCESS;
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
    if(osmp_shared == NULL || osmp_rank == -1) {
        fprintf(stderr, "OSMP_Finalize: Shared memory not initialized\n");
        return OSMP_FAILURE;
    }

    osmp_shared->pid_map[osmp_rank] = -1; // Setze den eigenen PID-Eintrag auf -1
    osmp_shared->free_ranks[osmp_shared->rear++] = osmp_rank; // Füge den Rang zu den freien Rängen hinzu
    osmp_shared->rear %= OSMP_MAX_PROCESSES; // Zirkuläre Queue

    //TODO: LOGGING durch OSMP_LOG
    fprintf(stderr, "OSMP_Finalize: Process %d finalized and released\n", osmp_rank);
    osmp_rank = -1; // Setze den Rang auf -1, um anzuzeigen, dass der Prozess nicht mehr aktiv ist

    return OSMP_GetSucess();
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

    // Überprüfen Sie, ob die Shared Memory-Struktur initialisiert ist
    if (!osmp_shared || !message) {
        fprintf(stderr, "OSMP_Log: Invalid parameters\n");
        return OSMP_FAILURE;
    }

    // Überprüfen Sie, ob der Prozess initialisiert ist
    if ((int)verbosity > osmp_shared->verbosity_level) {
        return OSMP_SUCCESS; // → Logging wird nicht ausgeführt, aber kein Fehler
    }

    // Loggen Sie die Nachricht in die Logdatei
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char log_line[512];

    int written = snprintf(log_line, sizeof(log_line),
                            "[%02d:%02d:%02d] PID %d: %s\n",
                            tm_info->tm_hour,
                            tm_info->tm_min,
                            tm_info->tm_sec,
                            getpid(),
                            message);

    if (written < 0 ) {
        fprintf(stderr, "OSMP_Log: Error writing to log file\n");
        return OSMP_FAILURE;
    }

    if(write(osmp_shared->log_fd, log_line, strlen(log_line)) < 0) {
        fprintf(stderr, "OSMP_Log: Error writing to log file\n");
        return OSMP_FAILURE;
    }

    return OSMP_SUCCESS;
}
