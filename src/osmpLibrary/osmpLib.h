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


#define OSMP_MAX_PROCESSES 256
#define SHM_NAME "/osmp_shm"
#define SHM_SIZE sizeof(osmp_shared_info_t) // Größe des Shared Memory

int OSMP_SetSharedMemory(void *ptr);

typedef struct {
    int process_count;

    char logfile_path[256];    // ← stattdessen: Pfad zur Logdatei
    int log_fd;                // ← stattdessen: File Descriptor für Logging
    int verbosity_level;

    pid_t pid_map[OSMP_MAX_PROCESSES];  // Index = Rank, Wert = PID (oder -1)

    int free_ranks[OSMP_MAX_PROCESSES]; // Queue für freie Ranks
    int front;  // Index des nächsten freien Ranks
    int rear;   // Index zum Einfügen eines neuen freien Ranks
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
