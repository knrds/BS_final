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
#include <stdlib.h>
#include <sys/stat.h>
#include <semaphore.h>

static osmp_shared_info_t *osmp_shared = NULL;
static int osmp_rank = -1;

MailboxTypeManagement *mailboxes;
FreeSlotQueue *fsq;
MessageType *slots;

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

    void *ptr = mmap(NULL, (size_t) shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

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


    //pointer auf Shared MemoryÜ
    const int N = osmp_shared->process_count;
    mailboxes =
            (MailboxTypeManagement *) (osmp_shared->pid_map + N);
    fsq =
            (FreeSlotQueue *) (mailboxes + N);
    slots =
            (MessageType *) (fsq + 1);

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


    //Race Condition vermeiden
    MailboxTypeManagement *mb = &mailboxes[dest];
    sem_wait(&mb->sem_free_mailbox_slots); // blockiert, wenn voll

    sem_wait(&fsq->sem_slots);
    pthread_mutex_lock(&fsq->free_slots_mutex);
    const int idx = fsq->free_slots[fsq->in_fsq];
    fsq->in_fsq = (uint16_t) (((int) fsq->in_fsq + 1) % OSMP_MAX_SLOTS);
    pthread_mutex_unlock(&fsq->free_slots_mutex);

    //Message Slot belegen
    slots[idx].datatype = datatype;
    slots[idx].count = count;
    slots[idx].source = osmp_rank;
    slots[idx].payload_length = payload_bytes;
    memcpy(slots[idx].payload, buf, payload_bytes);


    //In Ziel-MailboxTypeManagement einhängen

    pthread_mutex_lock(&mb->mailbox_mutex);

    // Ringpuffer schreiben
    mb->slot_indices[mb->in] = idx;
    mb->in = (mb->in + 1) % OSMP_MAX_SLOTS;

    pthread_mutex_unlock(&mb->mailbox_mutex);
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
    MailboxTypeManagement *mb = &mailboxes[osmp_rank];

    sem_wait(&mb->sem_msg_available);
    pthread_mutex_lock(&mb->mailbox_mutex);

    int idx = mb->slot_indices[mb->out];
    mb->out = (mb->out + 1) % OSMP_MAX_SLOTS;

    pthread_mutex_unlock(&mb->mailbox_mutex);
    sem_post(&mb->sem_free_mailbox_slots);


    const size_t payload_len = slots[idx].payload_length;
    const size_t nbytes = payload_len < max_bytes ? payload_len : max_bytes;
    memcpy(buf, slots[idx].payload, nbytes);

    *source = slots[idx].source;
    *len = (int) nbytes;

    //Slot-Index zurück in die FreeSlotQueue
    pthread_mutex_lock(&fsq->free_slots_mutex);
    fsq->free_slots[fsq->out_fsq] = idx;
    fsq->out_fsq = (uint16_t) (((int) fsq->out_fsq + 1) % OSMP_MAX_SLOTS);
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

    // Nur der Rank 0 zerstört die Synchronisationsobjekte
    if (osmp_rank == 0) {
        // FreeSlotQueue
        pthread_mutex_destroy(&fsq->free_slots_mutex);
        sem_destroy(&fsq->sem_slots);

        // Mailboxes
        for (int i = 0; i < osmp_shared->process_count; ++i) {
            sem_destroy(&mailboxes[i].sem_free_mailbox_slots);
            sem_destroy(&mailboxes[i].sem_msg_available);
            pthread_mutex_destroy(&mailboxes[i].mailbox_mutex);
        }

        // Log-Mutex
        sem_destroy(&osmp_shared->log_mutex);
    }

    // Shared Memory unmap
    if (munmap(osmp_shared, SHM_SIZE) == -1) {
        perror("munmap failed");
        return OSMP_FAILURE;
    }

    return OSMP_GetSucess();
}

/**
 * @brief Synchronisationsfunktion für alle OSMP-Prozesse.
 *
 * Diese Funktion blockiert den aufrufenden Prozess, bis alle anderen
 * OSMP-Prozesse ebenfalls OSMP_Barrier() aufgerufen haben.
 * Sobald alle beteiligt sind, werden sie gleichzeitig freigegeben.
 *
 * Intern wird eine gemeinsame barrier_t-Struktur im Shared Memory verwendet,
 * die mithilfe von pthread_mutex und pthread_cond Variablen implementiert ist.
 *
 * @return OSMP_SUCCESS bei erfolgreicher Synchronisation,
 *         OSMP_FAILURE bei Fehler (z. B. ungültige Barriere-Struktur)
 */
int OSMP_Barrier(void) {
    if (!osmp_shared ||
        osmp_shared->barrier.valid != BARRIER_VALID)
        return OSMP_FAILURE;

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Barrier: Process %d waiting at barrier", osmp_rank);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    //fprintf(stderr, "OSMP process %d is waiting at the barrier\n", osmp_rank);
    if (barrier_wait(&osmp_shared->barrier) == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "OSMP_Barrier: Process %d passed the barrier", osmp_rank);
        //fprintf(stderr, "OSMP process %d is PASSED at the barrier\n", osmp_rank);
        OSMP_Log(OSMP_LOG_BIB_CALL, msg);
        return OSMP_SUCCESS;
    }

    return OSMP_FAILURE;
}

/**
 * @brief Kollektive Kommunikationsfunktion zum Sammeln von Daten.
 *
 * Jeder OSMP-Prozess ruft diese Funktion auf, um Daten an einen gemeinsamen
 * Root-Prozess zu senden. Der Root sammelt die Daten aller Prozesse
 * (einschließlich seiner eigenen) in einem zusammenhängenden Empfangspuffer.
 *
 * Die Daten werden dort nach Rang sortiert gespeichert:
 * - Rang 0 → erster Block
 * - Rang 1 → zweiter Block
 * - ...
 *
 * @param sendbuf    Zeiger auf den lokalen Sendepuffer
 * @param sendcount  Anzahl der zu sendenden Elemente
 * @param sendtype   Datentyp der zu sendenden Elemente
 * @param recvbuf    Zeiger auf Empfangspuffer (nur Root)
 * @param recvcount  Anzahl der zu empfangenden Elemente pro Prozess
 * @param recvtype   Datentyp der empfangenen Elemente
 * @param root       Rang des Prozesses, der alle Daten empfängt
 *
 * @return OSMP_SUCCESS bei Erfolg, OSMP_FAILURE bei Fehler
 */
int OSMP_Gather(void *sendbuf, int sendcount, OSMP_Datatype sendtype,
                void *recvbuf, int recvcount, OSMP_Datatype recvtype,
                int root) {
    int rank, size, rv;

    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) return OSMP_FAILURE;
    rv = OSMP_Size(&size);
    if (rv == OSMP_FAILURE) return OSMP_FAILURE;

    if (root < 0 || root >= size)               return OSMP_FAILURE;

    unsigned int send_sz, recv_sz; // Größe der Datentypen wird geprüft
    if (OSMP_SizeOf(sendtype, &send_sz) != OSMP_SUCCESS) return OSMP_FAILURE;
    if (OSMP_SizeOf(recvtype, &recv_sz) != OSMP_SUCCESS) return OSMP_FAILURE;

    // Nicht-Root: senden und beenden
    if (rank != root) {
        return OSMP_Send(sendbuf, sendcount, sendtype, root) == OSMP_SUCCESS
             ? OSMP_SUCCESS : OSMP_FAILURE;
    }

    //Eigenen Beitrag kopieren
    if (!recvbuf) return OSMP_FAILURE;
    size_t own_bytes = (size_t)sendcount * send_sz;
    size_t max_bytes = (size_t)recvcount * recv_sz;
    if (own_bytes > max_bytes) own_bytes = max_bytes;
    size_t off0 = (size_t)root * (size_t)recvcount * recv_sz;  // Offset für den eigenen Beitrag
    memcpy((char*)recvbuf + off0, sendbuf, own_bytes);  // schreibt die eigene Nachricht in den Puffer an der stelle mit Offset

    //Rest empfangen via Zwischenspeicher
    size_t tmpbuf_size = (size_t)recvcount * recv_sz;  // Zwischenspeicher für die empfangenen Nachrichten
    char *tmpbuf = malloc(tmpbuf_size);
    if (!tmpbuf) return OSMP_FAILURE;

    for (int i = 0; i < size-1; ++i) {
        int src, len_bytes;
        rv = OSMP_Recv(tmpbuf, recvcount, recvtype, &src, &len_bytes);
        if (rv == OSMP_FAILURE) {
            free(tmpbuf);
            return OSMP_FAILURE;
        }
        size_t offset = (size_t)src * (size_t)recvcount * recv_sz;  // Offset für den eigenen Beitrag Src mal recvcount mal recv_sz
        memcpy((char*)recvbuf + offset, tmpbuf, (size_t)len_bytes);
    }

    free(tmpbuf);
    return OSMP_SUCCESS;
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
