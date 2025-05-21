//
// Created by chris on 19.05.2025.
//
#include "barrier.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>


/* * barrier_init - Initialisiert eine Barriere
 *
 * @barrier: Zeiger auf die Barriere
 * @count: Anzahl der Threads, die an der Barriere warten sollen
 *
 * Gibt 0 zurück, wenn erfolgreich, sonst einen Fehlercode.
 */
int barrier_init(barrier_t *barrier, int count) {
    if (barrier == NULL || count <= 0)
        return EINVAL;

    // Attribute für prozess-übergreifend
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    // Initialisiere Attribute
    pthread_mutexattr_init(&mattr); // erst initialisieren für setpshared
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED); // liegt im shared mamory und wird von mehreren Prozessen verwendet
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED); // funktioniert Prozessübergreifend

    // Barrier initialisieren
    pthread_mutex_init(&barrier->mutex, &mattr); //mutex tatsächlich zuweisen
    pthread_cond_init(&barrier->convar, &cattr);

    barrier->threshold = count;
    barrier->counter = count;
    barrier->cycle = 0;
    barrier->valid = BARRIER_VALID;
    return 0;
}

/* * barrier_destroy - Zerstört eine Barriere
 *
 * @barrier: Zeiger auf die Barriere
 *
 * Gibt 0 zurück, wenn erfolgreich, sonst einen Fehlercode.
 */
int barrier_destroy(barrier_t *barrier) {
    if (barrier == NULL || barrier->valid != BARRIER_VALID)
        return EINVAL;
    int s1 = pthread_mutex_destroy(&barrier->mutex);
    int s2 = pthread_cond_destroy(&barrier->convar);
    barrier->valid = 0;
    /* Gebe Rückgabecode zurück, falls einer fehlschlug */
    return s1 != 0 ? s1 : s2;
}

/* * barrier_wait - Wartet an der Barriere
 *
 * @barrier: Zeiger auf die Barriere
 *
 * Gibt 0 zurück, wenn erfolgreich, sonst einen Fehlercode.
 */
int barrier_wait(barrier_t *barrier) {
    int status, cancel, tmp, cycle;

    if (barrier == NULL || barrier->valid != BARRIER_VALID)
        return EINVAL;

    if ((status = pthread_mutex_lock(&barrier->mutex)) != 0)
        return status;  // Fehler beim Locken des Mutex

    cycle = barrier->cycle; // Aktuellen Zyklus speichern
    barrier->counter--; // Einen Thread dekrementieren

    if (barrier->counter == 0) {
        /* Letzter Thread: Zyklus hochzählen, counter zurücksetzen */
        barrier->cycle = cycle + 1;
        barrier->counter = barrier->threshold;  // Anzahl der Threads zurücksetzen
        /* Alle wartenden Threads freigeben */
        status = pthread_cond_broadcast(&barrier->convar); // Alle Threads aufwecken
        if (status != 0) { // Fehler beim Broadcast
            pthread_mutex_unlock(&barrier->mutex);
            return status;
        }
    } else {
        /* Wartepunkte sollen nicht abbrechbar sein */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);  // Verhintert Cancel des Threads während des Waits
        while (cycle == barrier->cycle && status == 0) {
            status = pthread_cond_wait(&barrier->convar, &barrier->mutex);  // Geht erst weiter wenn neuer cycle startet / cond broadcast kommt
        }
        /* Ursprünglichen Cancel-State wiederherstellen */
        pthread_setcancelstate(cancel, &tmp);
    }

    pthread_mutex_unlock(&barrier->mutex);
    return status;
}
