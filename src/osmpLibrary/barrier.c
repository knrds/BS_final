//
// Created by chris on 19.05.2025.
//
#include "barrier.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

#define BARRIER_VALID 0xBEEF

int barrier_init(barrier_t *barrier, int count) {
    if (barrier == NULL || count <= 0)
        return EINVAL;
    int s;
    if ((s = pthread_mutex_init(&barrier->mutex, NULL)) != 0)
        return s;
    if ((s = pthread_cond_init(&barrier->convar, NULL)) != 0) {
        pthread_mutex_destroy(&barrier->mutex);
        return s;
    }
    barrier->threshold = count;
    barrier->counter = count;
    barrier->cycle = 0;
    barrier->valid = BARRIER_VALID;
    return 0;
}


int barrier_destroy(barrier_t *barrier) {
    if (barrier == NULL || barrier->valid != BARRIER_VALID)
        return EINVAL;
    int s1 = pthread_mutex_destroy(&barrier->mutex);
    int s2 = pthread_cond_destroy(&barrier->convar);
    barrier->valid = 0;
    /* Gebe Rückgabecode zurück, falls einer fehlschlug */
    return s1 != 0 ? s1 : s2;
}


int barrier_wait(barrier_t *barrier) {
    int status, cancel, tmp, cycle;

    if (barrier == NULL || barrier->valid != BARRIER_VALID)
        return EINVAL;

    if ((status = pthread_mutex_lock(&barrier->mutex)) != 0)
        return status;

    cycle = barrier->cycle;
    barrier->counter--;

    if (barrier->counter == 0) {
        /* Letzter Thread: Zyklus hochzählen, counter zurücksetzen */
        barrier->cycle = cycle + 1;
        barrier->counter = barrier->threshold;
        /* Alle wartenden Threads freigeben */
        status = pthread_cond_broadcast(&barrier->convar);
        /* Hinweis: Fehler von broadcast ignorieren wir nicht */
        if (status != 0) {
            pthread_mutex_unlock(&barrier->mutex);
            return status;
        }
    } else {
        /* Wartepunkte sollen nicht abbrechbar sein */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
        while (cycle == barrier->cycle && status == 0) {
            status = pthread_cond_wait(&barrier->convar, &barrier->mutex);
        }
        /* Ursprünglichen Cancel-State wiederherstellen */
        pthread_setcancelstate(cancel, &tmp);
    }

    pthread_mutex_unlock(&barrier->mutex);
    return status;
}
