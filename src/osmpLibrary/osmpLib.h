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

#define SHM_NAME "/osmp_shm"
#define SHM_SIZE sizeof(osmp_shared_info_t) // Größe des Shared Memory

int OSMP_SetSharedMemory(void *ptr);

typedef struct {
    sem_t sem_empty; // how many messages you may still enqueue (≤ OSMP_MAX_MESSAGES_PROC)
    sem_t sem_full; // how many messages are currently enqueued
    sem_t mutex; // protects head/tail
    int head; // index of first MessageSlot in this mailbox (–1 if empty)
    int tail; // index of last  MessageSlot in this mailbox (–1 if empty)
} Mailbox;

//Eine nachricht
typedef struct {
    OSMP_Datatype datatype;
    int count;
    int source;
    size_t payload_length;
    int next; // link to next slot in the same mailbox
    char payload[OSMP_MAX_PAYLOAD_LENGTH];
} MessageSlot;

//Freie Slot Queue
typedef struct {
    uint16_t head, tail;
    sem_t sem_slots; // how many slots are free (≤ OSMP_MAX_SLOTS)
    sem_t mutex; // protects the head/tail
    int free_slots[OSMP_MAX_SLOTS];
} FreeSlotQueue;

typedef struct {
    int process_count;
    char logfile_path[256];
    int verbosity_level;
    pid_t pid_map[];
} osmp_shared_info_t;

#include "OSMP.h"

/**
 * @brief Dieses Makro wird verwendet, um den Compiler davon zu überzeugen, dass
 * eine Variable verwendet wird.
 *
 * @param x Die zu verwendende Variable
 */
#define UNUSED(x) (void)(x);

#endif // __OSMP_LIB_H__
