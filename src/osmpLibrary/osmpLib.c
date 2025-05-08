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
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <semaphore.h>

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

    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("fstat failed");
        close(shm_fd);
        return OSMP_FAILURE;
    }

    void *ptr = mmap(NULL, (size_t) shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); //TODO Shared memory pointer statt ptr*
    close(shm_fd);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return OSMP_FAILURE;
    }


    // Initialisiere den Shared Memory
    osmp_shared = (osmp_shared_info_t *) ptr;

    // Setze die Anzahl der Prozesse auf 0
    pid_t my_pid = getpid();
    osmp_rank = -1;

    // Durchsuche pid_map auf meine PID
    for (int i = 0; i < osmp_shared->process_count; i++) {
        if (osmp_shared->pid_map[i] == my_pid) {
            osmp_rank = i;
            break;
        }
    }

    if (osmp_rank == -1) {
        fprintf(stderr, "OSMP_Init: PID %d not found in pid_map\n", my_pid);
        return OSMP_FAILURE;
    }

    OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_Init() called");
    return OSMP_SUCCESS;
}

int OSMP_SetSharedMemory(void *ptr) {
    if (ptr == NULL) return OSMP_FAILURE;
    osmp_shared = (osmp_shared_info_t *) ptr;
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
    const int N = osmp_shared->process_count;
    if (dest < 0 || dest >= N || count < 0)
        return OSMP_FAILURE;

    unsigned int elem_size;
    if (OSMP_SizeOf(datatype, &elem_size) != OSMP_SUCCESS)
        return OSMP_FAILURE;

    if ((size_t) count > OSMP_MAX_PAYLOAD_LENGTH / elem_size)
        return OSMP_FAILURE;

    const size_t payload_bytes = (size_t) count * elem_size;

    //pointer auf Shared MemoryÜ
    MailboxTypeManagement *mailboxes =
            (MailboxTypeManagement *) (osmp_shared->pid_map + N);
    FreeSlotQueue *fsq =
            (FreeSlotQueue *) (mailboxes + N);
    MessageType *slots =
            (MessageType *) (fsq + 1);

    //Race Condition vermeiden
    MailboxTypeManagement *mb = &mailboxes[dest];
    sem_wait(&mb->sem_free_mailbox_slots); // blockiert, wenn voll

    sem_wait(&fsq->sem_slots);
    pthread_mutex_lock(&fsq->free_slots_mutex);
    const int idx = fsq->free_slots[fsq->head];
    fsq->head = (uint16_t) (((int) fsq->head + 1) % OSMP_MAX_SLOTS);
    pthread_mutex_unlock(&fsq->free_slots_mutex);

    //Message Slot belegen
    slots[idx].datatype = datatype;
    slots[idx].count = count;
    slots[idx].source = osmp_rank;
    slots[idx].payload_length = payload_bytes;
    memcpy(slots[idx].payload, buf, payload_bytes);


    //In Ziel-MailboxTypeManagement einhängen

    sem_wait(&mb->mailbox_mutex);

    // Ringpuffer schreiben
    mb->slot_indices[mb->in] = idx;
    mb->in = (mb->in + 1) % OSMP_MAX_SLOTS;

    sem_post(&mb->mailbox_mutex);
    sem_post(&mb->sem_msg_available);



    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Send: Process %d sent %lu bytes to process %d",
             osmp_rank, payload_bytes, dest);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    return OSMP_SUCCESS;
}

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source,
              int *len) {

    if (!buf || !source || !len || count < 0)
        return OSMP_FAILURE;
    const int N = osmp_shared->process_count;
    if (osmp_rank < 0 || osmp_rank >= N)
        return OSMP_FAILURE;

    unsigned int elem_size;
    if (OSMP_SizeOf(datatype, &elem_size) != OSMP_SUCCESS)
        return OSMP_FAILURE;

    if ((size_t) count > OSMP_MAX_PAYLOAD_LENGTH / elem_size)
        return OSMP_FAILURE;

    //Maximale Kopier-Bytezahl prüfen (Overflow / Puffer überschreiben)
    size_t max_bytes = (size_t) count * elem_size;

    // Pointer auf Shared Memory
    pid_t *pid_map = osmp_shared->pid_map;
    MailboxTypeManagement *mailboxes = (MailboxTypeManagement *) (pid_map + N);
    FreeSlotQueue *fsq = (FreeSlotQueue *) (mailboxes + N);
    MessageType *slots = (MessageType *) (fsq + 1);

    MailboxTypeManagement *mb = &mailboxes[osmp_rank];

    sem_wait(&mb->sem_msg_available);
    sem_wait(&mb->mailbox_mutex);

    int idx = mb->slot_indices[mb->out];
    mb->out = (mb->out + 1) % OSMP_MAX_SLOTS;

    sem_post(&mb->mailbox_mutex);
    sem_post(&mb->sem_free_mailbox_slots);


    const size_t payload_len = slots[idx].payload_length;
    const size_t nbytes = payload_len < max_bytes ? payload_len : max_bytes;
    memcpy(buf, slots[idx].payload, nbytes);

    *source = slots[idx].source;
    *len = (int) nbytes;

    //Slot-Index zurück in die FreeSlotQueue
    pthread_mutex_lock(&fsq->free_slots_mutex);
    fsq->free_slots[fsq->tail] = idx;
    fsq->tail = (uint16_t) (((int) fsq->tail + 1) % OSMP_MAX_SLOTS);
    pthread_mutex_unlock(&fsq->free_slots_mutex);
    sem_post(&fsq->sem_slots);

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Recv: Process %d received %lu bytes from process %d",
             osmp_rank, payload_len, *source);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    return OSMP_SUCCESS;
}

int OSMP_Finalize(void) {
    if (osmp_shared == NULL || osmp_rank == -1) {
        fprintf(stderr, "OSMP_Finalize: Shared memory not initialized\n");
        return OSMP_FAILURE;
    }

    osmp_shared->pid_map[osmp_rank] = -1; // Setze den eigenen PID-Eintrag auf -1

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Finalize: Process %d finalized and released", osmp_rank);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    if (munmap(osmp_shared, SHM_SIZE) == -1) {
        perror("munmap failed");
        return OSMP_FAILURE;
    }

    return OSMP_GetSucess();
}

int OSMP_Barrier(void) {
    // TODO: Implementieren Sie hier die Funktionalität der Funktion.
    // neuer af rsoll nicht barrier blockieren

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
    if ((int) verbosity > osmp_shared->verbosity_level) {
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

    if (written < 0) {
        fprintf(stderr, "OSMP_Log: Error writing to log file\n");
        return OSMP_FAILURE;
    }

    // Schreiben Sie die Logzeile in die Logdatei
    sem_wait(&osmp_shared->log_mutex);
    FILE *file = fopen(osmp_shared->logfile_path, "a");
    if (file == NULL) {
        fprintf(stderr, "OSMP_Log: Error writing to log file\n");
        return OSMP_FAILURE;
    }
    fprintf(file, "%s", log_line);
    fclose(file);
    sem_post(&osmp_shared->log_mutex);
    return OSMP_SUCCESS;
}
