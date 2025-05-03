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
#include <bits/semaphore.h>

#define SHM_NAME "/osmp_shm"
#define SHM_SIZE sizeof(osmp_shared_info_t) // Größe des Shared Memory


typedef struct Mailbox {
    void *buffer;       // Nachrichtenpuffer
    sem_t *mutex;       // Mutex für den Zugriff auf das Postfach
} Mailbox;



int OSMP_SetSharedMemory(void *ptr);

typedef struct {
    int process_count;
    char logfile_path[256];
    int verbosity_level;

    //IDEE
    sem_t *mailbox_empty;          // Semaphore für leere Postfächer
    sem_t *mailbox_full;           // Semaphore für volle Postfächer
    Mailbox **mailboxmap;          // Array von Zeigern auf Mailboxen, eine für jede PID


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
