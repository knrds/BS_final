/******************************************************************************
 * FILE: osmpLib.h
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * In dieser Datei sind zusätzliche (eigene) Hilfsfunktionen für die **interne**
 * Verwendung der OSMP-Bibliothek definiert.
 ******************************************************************************/

#ifndef __OSMP_LIB_H__
#define __OSMP_LIB_H__
#define OSMP_MAX_PROCESSES 256

typedef struct {
    int process_count; // Anzahl der Prozesse
    pid_t pids[OSMP_MAX_PROCESSES]; // Prozess-ID
    int is_active[OSMP_MAX_PROCESSES]; // Aktivitätsstatus des Prozesses
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
