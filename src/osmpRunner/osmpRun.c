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

struct thread_data {
  int thread_id;
  char *message;
};

void *thread_func(void *arg) {
  struct thread_data *data = (struct thread_data *)arg;
  printf("[Thread] [%lld] %s\n", timeInMilliseconds(), data->message);
  return NULL;
}

int main(void) {
  pthread_t thread;
  struct thread_data data;
  int rv;

  data.thread_id = 1;
  data.message = "Hallo Welt!";

  rv = pthread_create(&thread, NULL, thread_func, &data);
  if (0 != rv) {
    fprintf(stderr, "Fehler bei pthread_create\n");
    exit(EXIT_FAILURE);
  }

  printf("[Main] [%lld] %s\n", timeInMilliseconds(), data.message);

  rv = pthread_join(thread, NULL);
  if (0 != rv) {
    fprintf(stderr, "Fehler bei pthread_join\n");
    exit(EXIT_FAILURE);
  }

  return 0;
}
