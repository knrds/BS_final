/******************************************************************************
 * FILE: osmpRun.c
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * Diese Datei enthält die main Funktion für den OSMP-Runner.
 ******************************************************************************/

#include "osmpRun.h"

long long timeInMilliseconds(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

struct _thread_data_t {
  int thread_id;
  char *message;
};
typedef struct _thread_data_t thread_data_t;

void *thread_func(void *arg) {
  thread_data_t *thread_data = (thread_data_t *)arg;
  printf("[Thread] [%lld] %s\n", timeInMilliseconds(), thread_data->message);
  return NULL;
}

int main(void) {
  pthread_t thread;
  thread_data_t thread_data;
  int rv;

  printf("[Main]   [%lld] Beispielcode, der einen Thread startet und einen String ausgibt.\n", timeInMilliseconds());

  thread_data.thread_id = 1;
  thread_data.message = "Hallo Welt!";

  rv = pthread_create(&thread, NULL, thread_func, &thread_data);
  if (0 != rv) {
    fprintf(stderr, "Fehler bei pthread_create\n");
    exit(EXIT_FAILURE);
  }

  printf("[Main]   [%lld] %s\n", timeInMilliseconds(), thread_data.message);

  rv = pthread_join(thread, NULL);
  if (0 != rv) {
    fprintf(stderr, "Fehler bei pthread_join\n");
    exit(EXIT_FAILURE);
  }
  printf("[Main]   [%lld] Thread beendet\n", timeInMilliseconds());

  return 0;
}
