/******************************************************************************
 * FILE: osmpLib.h
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * In dieser Datei sind zusätzliche (eigene) Hilfsfunktionen für die **interne**
 * Verwendung der OSMP-Bibliothek definiert.
 ******************************************************************************/

#ifndef __OSMP_LIB_H__
#define __OSMP_LIB_H__

#include <sys/types.h>
#include <semaphore.h>
#include <stdint.h>
#include "OSMP.h"
#include "barrier.h"

#define SHM_NAME "/osmp_shm"
#define SHM_SIZE sizeof(osmp_shared_info_t) // Größe des Shared Memory

/**
 * enum für die verschiedenen Statuscodes einer OSMP-Anfrage
 */
# define CREATED 2
# define CURRENTLY_USED 3
/**
 * Datentyp zur Kapselung der Argumente für das nicht-blockierende Senden einer Nachricht
 */
typedef struct {
    const void *buffer;
    int count;
    OSMP_Datatype datatype;
    int destination;
} iSend_args;

/**
 * Datentyp zur Kapselung der Argumente für das nicht-blockierende Empfangen einer Nachricht
 */
typedef struct {
    void *buffer;
    int count;
    OSMP_Datatype datatype;
    int *source;
    int *len;
} iRecv_args;

/**
 * Datentyp zur Beschreibung einer OSMP-Anfrage.
 */
typedef struct {
    pthread_t thread;
    iSend_args *iSend_args;
    iRecv_args *iRecv_args;
    int status;
    sem_t status_semaphore;
} osmp_request;

int OSMP_SetSharedMemory(void *ptr);

// Mailboxen
typedef struct {
    sem_t sem_free_mailbox_slots; // how many messages you may still enqueue (≤ OSMP_MAX_MESSAGES_PROC)
    sem_t sem_msg_available; // how many messages are currently enqueued
    pthread_mutex_t mailbox_mutex; // protects in/out
    int slot_indices[OSMP_MAX_SLOTS];
    int in; // index of first MessageType in this mailbox (–1 if empty)
    int out; // index of last  MessageType in this mailbox (–1 if empty)
} MailboxTypeManagement;

//Eine nachricht
typedef struct {
    OSMP_Datatype datatype; // OSMP_UNSIGNED_CHAR...
    int count; // Count of the Dataobjects
    int source; // Who send this slot
    size_t payload_length; // Actual payload length
    char payload[OSMP_MAX_PAYLOAD_LENGTH];
} MessageType;

//Freie Slot Queue
typedef struct {
    uint16_t in_fsq, out_fsq; // oldest and newest msg
    sem_t sem_slots; // how many slots are free in general (≤ OSMP_MAX_SLOTS)
    pthread_mutex_t free_slots_mutex; // protects the in/out
    int free_slots[OSMP_MAX_SLOTS]; // SlotIds
} FreeSlotQueue;

typedef struct {
    int process_count;
    char logfile_path[256];
    int verbosity_level;
    sem_t log_mutex;
    barrier_t barrier;
    barrier_t barrier_gather;
    pid_t pid_map[];
} osmp_shared_info_t;

//TODO: Umstrukturierung der Shared Memory Bereiche

#include "OSMP.h"

/**
 * @brief Dieses Makro wird verwendet, um den Compiler davon zu überzeugen, dass
 * eine Variable verwendet wird.
 *
 * @param x Die zu verwendende Variable
 */
#define UNUSED(x) (void)(x);

#endif // __OSMP_LIB_H__
