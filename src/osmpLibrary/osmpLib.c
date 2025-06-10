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
static size_t shm_total_size = 0;

// Globale Instanz der dynamischen Bereichsadressen. Alle Funktionen greifen
// nach der Initialisierung über diese Zeiger auf die dynamischen Abschnitte zu.
dynamic_values dynamic_areas;

/**
 * Liefert die maximal unterstützte Nutzlastlänge einer Nachricht.
 */
int OSMP_GetMaxPayloadLength(void) {
    return OSMP_MAX_PAYLOAD_LENGTH;
}

/**
 * Anzahl der global verfügbaren Nachrichtenslots.
 */
int OSMP_GetMaxSlots(void) {
    return OSMP_MAX_SLOTS;
}

/**
 * Gibt zurück, wie viele Nachrichten maximal in einer Mailbox gespeichert
 * werden können.
 */
int OSMP_GetMaxMessagesProc(void) {
    return OSMP_MAX_MESSAGES_PROC;
}

/**
 * Hilfsfunktion: liefert den konstanten Fehlercode.
 */
int OSMP_GetFailure(void) {
    return OSMP_FAILURE;
}

/**
 * Hilfsfunktion: liefert den konstanten Erfolgscode.
 */
int OSMP_GetSucess(void) {
    return OSMP_SUCCESS;
}

/**
 * Initialisiert die Bibliothek und ermittelt die eigenen Zeiger auf den
 * Shared-Memory-Bereich. Jeder Prozess muss diese Funktion zu Beginn aufrufen.
 */
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

    shm_total_size = (size_t) shm_stat.st_size;
    void *ptr = mmap(NULL, shm_total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return OSMP_FAILURE;
    }


    // Initialisiere den Shared Memory
    osmp_shared = (osmp_shared_info_t *) ptr;

    // Adressen der dynamischen Bereiche ermitteln
    char *base = (char *) osmp_shared + SHM_HDR_SIZE;
    const int N = osmp_shared->process_count;
    dynamic_areas.pid_map = (pid_t *) base;
    dynamic_areas.mailboxes = (MailboxTypeManagement *) (dynamic_areas.pid_map + N);
    dynamic_areas.fsq = (FreeSlotQueue *) (dynamic_areas.mailboxes + N);
    dynamic_areas.slots = (MessageType *) (dynamic_areas.fsq + 1);
    dynamic_areas.gather_area = (char *) (dynamic_areas.slots + OSMP_MAX_SLOTS);

    // Bestimme eigenen Rang anhand der pid_map
    pid_t my_pid = getpid();
    osmp_rank = -1;

    for (int i = 0; i < N; i++) {
        if (dynamic_areas.pid_map[i] == my_pid) {
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

/**
 * Hinterlegt den Zeiger auf den bereits angelegten Shared-Memory-Bereich. Diese
 * Funktion wird nur vom Runner verwendet, bevor die Prozesse gestartet werden.
 */
int OSMP_SetSharedMemory(void *ptr) {
    if (ptr == NULL) return OSMP_FAILURE;
    osmp_shared = (osmp_shared_info_t *) ptr;
    return OSMP_SUCCESS;
}


/**
 * Ermittelt die Größe des übergebenen Datentyps in Byte.
 */
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

/**
 * Liefert in *size* die Anzahl aller gestarteten Prozesse.
 */
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


/**
 * Schreibt den eigenen Rang in *rank*.
 */
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


/**
 * Sendet eine Nachricht an den Prozess mit der Nummer dest.
 * Die Funktion blockiert solange, bis der Nachrichtenpuffer einen freien Slot
 * besitzt und die Daten kopiert wurden.
 */
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
    MailboxTypeManagement *mb = &dynamic_areas.mailboxes[dest];
    sem_wait(&mb->sem_free_mailbox_slots); // blockiert, wenn voll

    sem_wait(&dynamic_areas.fsq->sem_slots);
    pthread_mutex_lock(&dynamic_areas.fsq->free_slots_mutex);
    const int idx = dynamic_areas.fsq->free_slots[dynamic_areas.fsq->in_fsq];
    dynamic_areas.fsq->in_fsq = (uint16_t) (((int) dynamic_areas.fsq->in_fsq + 1) % OSMP_MAX_SLOTS);
    pthread_mutex_unlock(&dynamic_areas.fsq->free_slots_mutex);

    //Message Slot belegen
    dynamic_areas.slots[idx].datatype = datatype;
    dynamic_areas.slots[idx].count = count;
    dynamic_areas.slots[idx].source = osmp_rank;
    dynamic_areas.slots[idx].payload_length = payload_bytes;
    memcpy(dynamic_areas.slots[idx].payload, buf, payload_bytes);


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

/**
 * Wartet auf eine eingehende Nachricht und kopiert sie in den übergebenen
 * Puffer. Die Funktion blockiert, bis eine Nachricht vorliegt.
 */
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
    MailboxTypeManagement *mb = &dynamic_areas.mailboxes[osmp_rank];

    sem_wait(&mb->sem_msg_available);
    pthread_mutex_lock(&mb->mailbox_mutex);

    int idx = mb->slot_indices[mb->out];
    mb->out = (mb->out + 1) % OSMP_MAX_SLOTS;

    pthread_mutex_unlock(&mb->mailbox_mutex);
    sem_post(&mb->sem_free_mailbox_slots);


    const size_t payload_len = dynamic_areas.slots[idx].payload_length;
    const size_t nbytes = payload_len < max_bytes ? payload_len : max_bytes;
    memcpy(buf, dynamic_areas.slots[idx].payload, nbytes);

    *source = dynamic_areas.slots[idx].source;
    *len = (int) nbytes;

    //Slot-Index zurück in die FreeSlotQueue
    pthread_mutex_lock(&dynamic_areas.fsq->free_slots_mutex);
    dynamic_areas.fsq->free_slots[dynamic_areas.fsq->out_fsq] = idx;
    dynamic_areas.fsq->out_fsq = (uint16_t) (((int) dynamic_areas.fsq->out_fsq + 1) % OSMP_MAX_SLOTS);
    pthread_mutex_unlock(&dynamic_areas.fsq->free_slots_mutex);
    sem_post(&dynamic_areas.fsq->sem_slots);

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Recv: Process %d received %lu bytes from process %d",
             osmp_rank, payload_len, *source);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    return OSMP_SUCCESS;
}

/**
 * Gibt alle Ressourcen frei und beendet die Nutzung des Shared Memory.
 */
int OSMP_Finalize(void) {
    if (osmp_shared == NULL || osmp_rank == -1) {
        fprintf(stderr, "OSMP_Finalize: Shared memory not initialized\n");
        return OSMP_FAILURE;
    }

    dynamic_areas.pid_map[osmp_rank] = -1; // Setze den eigenen PID-Eintrag auf -1

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Finalize: Process %d finalized and released", osmp_rank);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    // Nur der Rank 0 zerstört die Synchronisationsobjekte
    if (osmp_rank == 0) {
        // FreeSlotQueue
        pthread_mutex_destroy(&dynamic_areas.fsq->free_slots_mutex);
        sem_destroy(&dynamic_areas.fsq->sem_slots);

        // Mailboxes
        for (int i = 0; i < osmp_shared->process_count; ++i) {
            sem_destroy(&dynamic_areas.mailboxes[i].sem_free_mailbox_slots);
            sem_destroy(&dynamic_areas.mailboxes[i].sem_msg_available);
            pthread_mutex_destroy(&dynamic_areas.mailboxes[i].mailbox_mutex);
        }

        // Log-Mutex
        sem_destroy(&osmp_shared->log_mutex);
    }

    // Shared Memory unmap
    if (munmap(osmp_shared, shm_total_size) == -1) {
        perror("munmap failed");
        return OSMP_FAILURE;
    }

    return OSMP_GetSucess();
}

/**
 * Synchronisationspunkt für alle Prozesse. Erst wenn alle Prozesse diese
 * Funktion erreicht haben, geht es für alle weiter.
 */
int OSMP_Barrier(void) {
    if (!osmp_shared ||
        osmp_shared->barrier.valid != BARRIER_VALID)
        return OSMP_FAILURE;

    char msg[128];
    snprintf(msg, sizeof(msg), "OSMP_Barrier: Process %d waiting at barrier", osmp_rank);
    OSMP_Log(OSMP_LOG_BIB_CALL, msg);

    if (barrier_wait(&osmp_shared->barrier) == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "OSMP_Barrier: Process %d passed the barrier", osmp_rank);
        OSMP_Log(OSMP_LOG_BIB_CALL, msg);
        return OSMP_SUCCESS;
    }

    return OSMP_FAILURE;
}

/**
 * Sammelt Daten aller Prozesse beim Root-Prozess.
 * Jeder Nicht-Root sendet seine Daten, der Root empfängt anschließend alle
 * Nachrichten und legt sie in recvbuf ab.
 */
int OSMP_Gather(void *sendbuf, int sendcount, OSMP_Datatype sendtype,
                void *recvbuf, int recvcount, OSMP_Datatype recvtype,
                int root) {
    int rank, size, rv;

    rv = OSMP_Rank(&rank);
    if (rv == OSMP_FAILURE) return OSMP_FAILURE;
    rv = OSMP_Size(&size);
    if (rv == OSMP_FAILURE) return OSMP_FAILURE;

    if (root < 0 || root >= size) return OSMP_FAILURE;

    unsigned int send_sz, recv_sz;
    if (OSMP_SizeOf(sendtype, &send_sz) != OSMP_SUCCESS) return OSMP_FAILURE;
    if (OSMP_SizeOf(recvtype, &recv_sz) != OSMP_SUCCESS) return OSMP_FAILURE;

    // Nicht-Root: senden und beenden
    if (rank != root) {
        return OSMP_Send(sendbuf, sendcount, sendtype, root) == OSMP_SUCCESS
                   ? OSMP_SUCCESS
                   : OSMP_FAILURE;
    }

    //Eigenen Beitrag kopieren
    if (!recvbuf) return OSMP_FAILURE;
    size_t own_bytes = (size_t) sendcount * send_sz;
    size_t max_bytes = (size_t) recvcount * recv_sz;
    if (own_bytes > max_bytes) own_bytes = max_bytes;
    size_t off0 = (size_t) root * (size_t) recvcount * recv_sz;
    memcpy((char *) recvbuf + off0, sendbuf, own_bytes);

    //Rest empfangen via Zwischenspeicher
    size_t tmpbuf_size = (size_t) recvcount * recv_sz;
    char *tmpbuf = malloc(tmpbuf_size);
    if (!tmpbuf) return OSMP_FAILURE;

    for (int i = 0; i < size - 1; ++i) {
        int src, len_bytes;
        rv = OSMP_Recv(tmpbuf, recvcount, recvtype, &src, &len_bytes);
        if (rv == OSMP_FAILURE) {
            free(tmpbuf);
            return OSMP_FAILURE;
        }
        size_t offset = (size_t) src * (size_t) recvcount * recv_sz;
        memcpy((char *) recvbuf + offset, tmpbuf, (size_t) len_bytes);
    }

    free(tmpbuf);
    return OSMP_SUCCESS;
}

/**
 * implements the semaphore waiting function
 * @param sem the semaphore on which to wait
 * @return OSMP_SUCCESS if the wait is done and successful otherwise OSMP_FAILURE
 */
int semwait(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        if (OSMP_Log(OSMP_LOG_BIB_CALL, "Fehler: semwait") == OSMP_FAILURE) {
            perror("log_message Failed");
        }
        return OSMP_FAILURE;
    }
    return OSMP_SUCCESS;
}

/**
 * implements the semaphore signal function
 * @param sem the semaphore on what it signals
 * @return OSMP_SUCCESS if the Signal is done and successful otherwise OSMP_FAILURE
 */
int semsignal(sem_t *sem) {
    if (sem_post(sem) == -1) {
        if (OSMP_Log(OSMP_LOG_BIB_CALL, "Fehler: semsignal") == OSMP_FAILURE) {
            perror("log_message Failed");
        }
        return OSMP_FAILURE;
    }
    return OSMP_SUCCESS;
}

/**
 * Initialisieren eines Threads für das blockierende Senden einer Nachricht
 * @param arg Request in denen Mutex und Argumente zum Senden enthalten sind (wird intern gecasted)
 * @return Status der Verarbeitung OSMP_SUCCESS bei Erfolg, OSMP_FAILURE sonst [muss als int gecasted werden beim auslesen]
 */
void *init_send_thread(void *arg) {
    osmp_request *request = (osmp_request *) arg;
    iSend_args *send_arguments = request->iSend_args;

    int status = OSMP_Send(send_arguments->buffer, send_arguments->count, send_arguments->datatype,
                           send_arguments->destination);

    semwait(&request->status_semaphore);
    request->status = status;
    semsignal(&request->status_semaphore);

    if (status != OSMP_SUCCESS) {
        pthread_exit((void *) OSMP_FAILURE);
    }

    pthread_exit((void *) OSMP_SUCCESS);
}


/**
 * Initialisieren eines Threads für das blockierende Empfangens einer Nachricht
 * @param arg die osmp_request
 * @return OSMP_SUCCESS bei Erfolg, OSMP_FAILURE sonst
 */
void *init_recv_thread(void *arg) {
    osmp_request *request = (osmp_request *) arg;
    iRecv_args *recv_arguments = request->iRecv_args;

    int size;
    OSMP_Size(&size);
    int status = OSMP_Recv(recv_arguments->buffer, recv_arguments->count, recv_arguments->datatype,
                           recv_arguments->source, recv_arguments->len);
    semwait(&request->status_semaphore);
    request->status = status;
    semsignal(&request->status_semaphore);
    if (status != OSMP_SUCCESS) {
        pthread_exit((void *) OSMP_FAILURE);
    }
    pthread_exit((void *) OSMP_SUCCESS);
}

/**
 * Initialisert einen Thread für die asynchrone Senden und Empfangen einer Nachricht .
 * @param request OSMP_Request, für welche der Thread erstellt werden soll.
 * @param function Funktion, welche vom Thread gestartet wird
 * @return OSMP_SUCCESS bei Erfolg, sonst OSMP_FAILURE
 */
int initialize_thread(osmp_request *request, void *(*function)(void *)) {
    pthread_t thread;
    pthread_attr_t thread_attr;

    int status = pthread_attr_init(&thread_attr);

    if (status != 0) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "initialize_joinable_thread_for_request: Error initializing thread");
        return OSMP_FAILURE;
    }

    // Sorgt dafür, dass der Thread joinable ist, damit später mit pthread_join() gewartet werden kann
    status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    if (status != 0) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "initialize_joinable_thread_for_request: Error setting detach state in thread");
        return OSMP_FAILURE;
    }

    // Setze den Scope des Threads auf SYSTEM, damit er auf dem System-Scheduler läuft
    status = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);

    if (status != 0) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "initialize_joinable_thread_for_request: Error setting scope of thread");
        return OSMP_FAILURE;
    }
    status = pthread_create(&thread, &thread_attr, function, request);

    if (status != 0) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "initialize_joinable_thread_for_request: Error creating thread");
        return OSMP_FAILURE;
    }

    pthread_attr_destroy(&thread_attr);
    request->thread = thread;

    return OSMP_SUCCESS;
}


/**
 * Nicht blockierendes Senden einer Nachricht. Die eigentliche Sendearbeit wird
 * in einem separaten Thread erledigt.
 */
int OSMP_ISend(const void *buf, int count, OSMP_Datatype datatype, int dest, OSMP_Request request) {
    iSend_args *arguments = calloc(1, sizeof(iSend_args));
    arguments->buffer = buf;
    arguments->count = count;
    arguments->datatype = datatype;
    arguments->destination = dest;

    osmp_request *proper_request = (osmp_request *) request;

    semwait(&proper_request->status_semaphore);
    if (proper_request->status != CREATED) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_ISend: Request must not be used twice or was not initialized...");
        semsignal(&proper_request->status_semaphore);
        return OSMP_FAILURE;
    }
    proper_request->status = CURRENTLY_USED;
    semsignal(&proper_request->status_semaphore);

    proper_request->iSend_args = arguments;

    if (initialize_thread(proper_request, &init_send_thread) != OSMP_SUCCESS) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_ISend: Error initializing send thread");
        free(arguments);
        return OSMP_FAILURE;
    }

    return OSMP_SUCCESS;
}

/**
 * Nicht blockierendes Empfangen. Das eigentliche Warten erfolgt in einem
 * Hilfsthread, sodass der Aufrufer sofort weiterarbeiten kann.
 */
int OSMP_IRecv(void *buf, int count, OSMP_Datatype datatype, int *source,
               int *len, OSMP_Request request) {
    iRecv_args *arguments = calloc(1, sizeof(iRecv_args));
    arguments->buffer = buf;
    arguments->count = count;
    arguments->datatype = datatype;
    arguments->source = source;
    arguments->len = len;

    osmp_request *req = (osmp_request *) request;

    semwait(&req->status_semaphore);
    if (req->status != CREATED) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_IRecv: Request must not be used twice or was not initialized...");
        semsignal(&req->status_semaphore);
        return OSMP_FAILURE;
    }
    req->status = CURRENTLY_USED;
    semsignal(&req->status_semaphore);

    req->iRecv_args = arguments;

    if (initialize_thread(req, &init_recv_thread) != OSMP_SUCCESS) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_ISend: Error initializing send thread");
        free(arguments);
        return OSMP_FAILURE;
    }
    return OSMP_SUCCESS;
}

/**
 * Prüft nicht blockierend, ob die angegebene Request bereits abgeschlossen ist.
 */
int OSMP_Test(OSMP_Request request, int *flag) {
    osmp_request *req = (osmp_request *) request;
    semwait(&req->status_semaphore);
    if (req->status == CREATED || req->status == CURRENTLY_USED) {
        *flag = 0;
        semsignal(&req->status_semaphore);
        return OSMP_SUCCESS;
    }

    *flag = 1;
    semsignal(&req->status_semaphore);
    return OSMP_SUCCESS;
}

/**
 * Blockiert, bis die nicht blockierende Operation in *request* abgeschlossen
 * ist.
 */
int OSMP_Wait(OSMP_Request request) {
    osmp_request *proper_request = (osmp_request *) request;

    void *thread_status = (void *) OSMP_SUCCESS;
    int status = pthread_join(proper_request->thread, &thread_status);

    if (status != 0) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_Wait: Error joining thread");
        return OSMP_FAILURE;
    }

    if (thread_status != (void *) OSMP_SUCCESS) {
        OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_Wait: Thread did not complete successfully");
        return OSMP_FAILURE;
    }
    return OSMP_SUCCESS;
}

/**
 * Legt eine neue OSMP_Request an, die für nicht blockierende Operationen
 * verwendet werden kann.
 */
int OSMP_CreateRequest(OSMP_Request *request) {
    OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_CreateRequest() called");
    osmp_request *req = calloc(1, sizeof(osmp_request));
    req->status = CREATED;
    sem_init(&req->status_semaphore, 0, 1);
    *request = req;
    return OSMP_SUCCESS;
}

/**
 * Gibt die Ressourcen einer zuvor angelegten Request wieder frei.
 */
int OSMP_RemoveRequest(OSMP_Request *request) {
    OSMP_Log(OSMP_LOG_BIB_CALL, "OSMP_RemoveRequest() called");
    osmp_request *req = (osmp_request *) *request;
    sem_destroy(&req->status_semaphore);
    free(req);
    *request = NULL;
    return OSMP_SUCCESS;
}

/**
 * Schreibt eine Lognachricht in die von osmprun angegebene Datei, sofern das
 * aktuelle Verbositätslevel dies zulässt.
 */
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
