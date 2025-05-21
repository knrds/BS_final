//
// Created by chris on 19.05.2025.
//

#include <sys/types.h>
#include <semaphore.h>
#include <stdint.h>

#ifndef BARRIER_H
#define BARRIER_H

#define BARRIER_VALID 0xBEEF

typedef struct {
    pthread_mutex_t mutex; /* Zugriffskontrolle beim verändern der Variablen */
    pthread_cond_t convar; /* Warten an der Barriere */
    int valid; /* gesetzt, wenn Barriere initalisiert */
    int threshold; /* Anzahl der gesamten erwarteten Threads */
    int counter; /* die noch fehlenden Threads */
    int cycle; /* Flag ob Barriere aktiv ist - Verhindert Durchlaufen von Prozessen aus früheren Durchläufen */
} barrier_t;

int barrier_init (barrier_t *barrier, int count);
int barrier_destroy (barrier_t *barrier);
int barrier_wait (barrier_t *barrier);

#endif //BARRIER_H
