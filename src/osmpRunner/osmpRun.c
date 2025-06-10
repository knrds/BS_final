//
// Erstellt von knrd am 02.04.25.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <getopt.h>

#include "osmpRun.h"
#include "../osmpLibrary/barrier.h"

#include <pthread.h>

#include "../osmpLibrary/OSMP.h"
#include "../osmpLibrary/osmpLib.h"

char *logfile_path = NULL; // Pfad zur Logdatei
int verbosity_level = 1; // Standard: Level 1
char buffer[256]; // Puffer für Log-Meldungen

osmp_shared_info_t *osmp_shared = NULL; // globaler Zeiger

/**
 * Legt den Shared Memory entsprechend der Prozessanzahl an und initialisiert
 * alle Synchronisationsobjekte.
 */
int setup_shared_memory(int process_count) {
    size_t sz_hdr = sizeof(osmp_shared_info_t);
    size_t sz_pidmap = (size_t) process_count * sizeof(pid_t);
    size_t sz_mailbx = (size_t) process_count * sizeof(MailboxTypeManagement);
    size_t sz_fsq = sizeof(FreeSlotQueue);
    size_t sz_slots = OSMP_MAX_SLOTS * sizeof(MessageType);
    size_t sz_gather = (size_t) process_count * OSMP_MAX_PAYLOAD_LENGTH;

    size_t shm_size = sz_hdr
                      + sz_pidmap
                      + sz_mailbx
                      + sz_fsq
                      + sz_slots
                      + sz_gather;

    shm_unlink(SHM_NAME);
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Error creating shared memory");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, (off_t) shm_size) == -1) {
        perror("Error setting size of shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    void *ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    OSMP_SetSharedMemory(ptr);
    osmp_shared = (osmp_shared_info_t *) ptr;
    memset(osmp_shared, 0, shm_size);

    osmp_shared->process_count = process_count;
    osmp_shared->verbosity_level = verbosity_level;

    if (logfile_path != NULL) {
        strncpy(osmp_shared->logfile_path, logfile_path, sizeof(osmp_shared->logfile_path) - 1);
        osmp_shared->logfile_path[sizeof(osmp_shared->logfile_path) - 1] = '\0';
    } else {
        osmp_shared->logfile_path[0] = '\0';
    }

    // Adressen der dynamischen Bereiche berechnen und zentral speichern
    char *base = (char *)osmp_shared + SHM_HDR_SIZE;
    dynamic_areas.pid_map = (pid_t *) base;
    dynamic_areas.mailboxes = (MailboxTypeManagement *) (dynamic_areas.pid_map + process_count);
    dynamic_areas.fsq = (FreeSlotQueue *) (dynamic_areas.mailboxes + process_count);
    dynamic_areas.slots = (MessageType *) (dynamic_areas.fsq + 1);
    dynamic_areas.gather_area = (char *) (dynamic_areas.slots + OSMP_MAX_SLOTS);

    for (int i = 0; i < process_count; i++)
        dynamic_areas.pid_map[i] = -1;

    // Initialisierung
    sem_init(&osmp_shared->log_mutex, 1, 1); // 1 = für Shared Memory


    // Mailboxes anschließen
    MailboxTypeManagement *mailboxes = dynamic_areas.mailboxes;

    for (int i = 0; i < process_count; i++) {
        sem_init(&mailboxes[i].sem_free_mailbox_slots, 1, OSMP_MAX_MESSAGES_PROC); // freie Plätze
        sem_init(&mailboxes[i].sem_msg_available, 1, 0);
        // empfangbare Nachrichten
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        // z.B. Fehler-Checking einschalten
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        // oder: für process-shared
        // pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

        pthread_mutex_init(&mailboxes[i].mailbox_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        // Zugriffsschutz (binär)

        mailboxes[i].in = 0; // oder: in_fsq
        mailboxes[i].out = 0; // oder: out_fsq

        for (int j = 0; j < OSMP_MAX_SLOTS; j++) {
            mailboxes[i].slot_indices[j] = -1; // optional zur Debughilfe
        }
    }

    // FreeSlotQueue anschließen
    FreeSlotQueue *fsq = dynamic_areas.fsq;
    fsq->in_fsq = fsq->out_fsq = 0;
    sem_init(&fsq->sem_slots, 1, OSMP_MAX_SLOTS);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&fsq->free_slots_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    for (int i = 0; i < OSMP_MAX_SLOTS; i++)
        fsq->free_slots[fsq->out_fsq++] = i;

    if(barrier_init(&osmp_shared->barrier, process_count) != 0) {
        perror("Barrier initialization failed");
        close(fd);
        return OSMP_FAILURE;
    }

    if(barrier_init(&osmp_shared->barrier_gather, process_count) != 0) {
        perror("Gather Barrier initialization failed");
        close(fd);
        return OSMP_FAILURE;
    }

    close(fd);
    return 0;
}


/**
 * Startet die gewünschte Anzahl an OSMP-Prozessen und wartet auf deren Ende.
 */
int main(int argc, char *argv[]) {
    int opt; // Variable für Befehlszeilenoptionen
    int process_count = -1; // Anzahl der zu startenden Prozesse
    char *executable_path = NULL; // Pfad zur ausführbaren Datei
    char **exec_args = NULL; // Argumente für die ausführbare Datei
    int started_count = 0; // Zähler für gestartete Prozesse


    // Analysiere Befehlszeilenoptionen
    while ((opt = getopt(argc, argv, "p:l:v:e:")) != -1) {
        switch (opt) {
            // -p: Anzahl der zu startenden Prozesse
            case 'p': {
                char *endptr1;
                process_count = (int) strtol(optarg, &endptr1, 10); // Wandle String in long um
                if (*endptr1 != '\0' || process_count <= 0) {
                    fprintf(stderr, "Invalid process count: %s\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;
            // -l: Pfad zur Logdatei
            case 'l':
                logfile_path = optarg;
                break;
            // -v: Verbositätslevel
            case 'v': {
                char *endptr;
                verbosity_level = (int) strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || verbosity_level < 1 || verbosity_level > 3) {
                    fprintf(stderr, "Invalid Log-Level: %s\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            break;
            // -e: Pfad zur ausführbaren Datei
            case 'e':
                executable_path = optarg;
                exec_args = &argv[optind - 1]; // Setze den Pfad der ausführbaren Datei ohne "-e"
                exec_args[0] = executable_path; // Setze den Pfad der ausführbaren Datei korrekt
                break;
            // Ungültige Option - Nutzungshinweis
            default:
                fprintf(
                    stderr,
                    "Usage: %s [-p process_count] [-l logfile_path] [-v verbosity_level] [-e executable_path]\n",
                    argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Überprüfe, ob die Anzahl der zu startenden Prozesse und der Pfad zur ausführbaren Datei angegeben sind
    if (process_count <= 0 || executable_path == NULL) {
        fprintf(stderr, "Error: Number of processes to start and executable path must be provided.\n");
        return EXIT_FAILURE;
    }

    if (setup_shared_memory(process_count) != 0) {
        fprintf(stderr, "Shared Memory Setup failed\n");
        return EXIT_FAILURE;
    }

    // Leere Logdatei, falls vorhanden
    if (logfile_path) fclose(fopen(logfile_path, "w"));

    int log_fd = open(logfile_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (log_fd == -1) {
        perror("Could not open log file");
        return EXIT_FAILURE;
    }
    strncpy(osmp_shared->logfile_path, logfile_path, sizeof(osmp_shared->logfile_path) - 1);
    osmp_shared->logfile_path[sizeof(osmp_shared->logfile_path) - 1] = '\0'; // Null-terminator setzen


    // Starte Logging
    snprintf(buffer, sizeof(buffer), "Starting %d instances of %s", process_count, executable_path);
    OSMP_Log(OSMP_LOG_BIB_CALL, buffer);

    pid_t pid_children[process_count]; // Array zur Speicherung der Kinderprozess-IDs

    for (int i = 0; i < process_count; i++) {

        int start_pipe[2];
        pipe(start_pipe);


        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            OSMP_Log(OSMP_LOG_FAILS, "Fork failed");
            continue;
        } else if (pid == 0) {
            // Kindprozess – führt nur exec() aus
            close(start_pipe[1]); // Schreib-Ende schließen
            char dummy;
            read(start_pipe[0], &dummy, 1); // warten auf Startsignal

            execvp(executable_path, exec_args);

            // Nur erreichbar, wenn exec scheitert
            perror("Exec failed");
            OSMP_Log(OSMP_LOG_FAILS, "Exec failed");
            exit(EXIT_FAILURE);
        } else {
            // Nur hier im Elternprozess die pid_map schreiben
            dynamic_areas.pid_map[i] = pid;

            close(start_pipe[0]); // Lese-Ende schließen
            write(start_pipe[1], "x", 1); // Startsignal senden


            snprintf(buffer, sizeof(buffer), "Started instance %d with PID %d (Rank %d)", i + 1, pid, i);
            pid_children[started_count++] = pid;
            OSMP_Log(OSMP_LOG_BIB_CALL, buffer);
        }
    }


    // Warte, bis alle Kindprozesse beendet sind
    for (int i = 0; i < started_count; i++) {
        int status;
        pid_t pid = waitpid(pid_children[i], &status, 0);

        if (pid == -1) {
            perror("Error: Wait failed");
            OSMP_Log(OSMP_LOG_FAILS, "Wait failed");
        } else {
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                snprintf(buffer, sizeof(buffer), "Child process %d exited with code %d", pid, exit_status);
                OSMP_Log(OSMP_LOG_BIB_CALL, buffer);
            } else {
                snprintf(buffer, sizeof(buffer), "Child process %d terminated abnormally with code %d", pid, status);
                OSMP_Log(OSMP_LOG_FAILS, buffer);
            }
        }
    }

    shm_unlink(SHM_NAME);

    if (log_fd != -1) {
        close(log_fd);
    }
    return EXIT_SUCCESS;
}
