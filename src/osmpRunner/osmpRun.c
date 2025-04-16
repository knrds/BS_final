//
// Erstellt von knrd am 02.04.25.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // ✅ für ftruncate, fork, getopt, etc.
#include <string.h>     // ✅ für memset, snprintf
#include <fcntl.h>      // ✅ für O_CREAT, O_RDWR, shm_open
#include <sys/mman.h>   // ✅ für mmap
#include <sys/stat.h>   // ✅ für Mode-Typen (0666 etc.)
#include <sys/types.h>  // ✅ für pid_t
#include <sys/wait.h>   // ✅ für waitpid
#include <time.h>       // ✅ für time()
#include <getopt.h>     // ✅ optional (eigentlich reicht <unistd.h> für getopt)

#include "osmpRun.h"
#include "../osmpLibrary/OSMP.h"
#include "../osmpLibrary/osmpLib.h"

char *logfile_path = NULL; // Pfad zur Logdatei
int verbosity_level = 1;  // Standard: Level 1
char buffer[256]; // Puffer für Log-Meldungen

osmp_shared_info_t *osmp_shared = NULL; // globaler Zeiger

int setup_shared_memory(void){
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666); // Erstelle Shared-Memory-Objekt

    if (fd == -1) {
        perror("Error creating shared memory");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("Error setting size of shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    OSMP_SetSharedMemory(ptr);

    if (ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    osmp_shared = (osmp_shared_info_t *)ptr;
    memset(osmp_shared, 0, SHM_SIZE); // Nullinitialisierung

    // Initialisiere Prozessabbildung und freie Ränge-Warteschlange
    // Queue leer starten
    osmp_shared->front = 0;
    osmp_shared->rear = 0;
    osmp_shared->free_rank_count = OSMP_MAX_PROCESSES;

    // Befülle Queue mit allen freien Ranks
    for (int i = 0; i < OSMP_MAX_PROCESSES; i++) {
        osmp_shared->free_ranks[osmp_shared->rear++] = i;
        osmp_shared->pid_map[i] = -1;
    }


    // Setze den Pfad der Logdatei und das Verbositätslevel
    if (logfile_path != NULL) {
        strncpy(osmp_shared->logfile_path, logfile_path, sizeof(osmp_shared->logfile_path) - 1);
        osmp_shared->logfile_path[sizeof(osmp_shared->logfile_path) - 1] = '\0'; // Sicherer Null-Terminator
    } else {
        osmp_shared->logfile_path[0] = '\0'; // leer lassen
    }

    osmp_shared->verbosity_level = verbosity_level;

    close(fd); // Dateideskriptor kann geschlossen werden
    return 0;
    }


int main(int argc, char *argv[]) {
    int opt; // Variable für Befehlszeilenoptionen
    int process_count = -1; // Anzahl der zu startenden Prozesse
    char *executable_path = NULL; // Pfad zur ausführbaren Datei
    char **exec_args = NULL; // Argumente für die ausführbare Datei
    int started_count = 0; // Zähler für gestartete Prozesse

    if (setup_shared_memory() != 0) {
        fprintf(stderr, "Shared Memory Setup failed\n");
        return EXIT_FAILURE;
    }

    // Analysiere Befehlszeilenoptionen
    while((opt = getopt(argc, argv, "p:l:v:e:")) != -1) {
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
            case 'v':{
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
                fprintf(stderr, "Usage: %s [-p process_count] [-l logfile_path] [-v verbosity_level] [-e executable_path]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Überprüfe, ob die Anzahl der zu startenden Prozesse und der Pfad zur ausführbaren Datei angegeben sind
    if (process_count <= 0 || executable_path == NULL) {
        fprintf(stderr, "Error: Number of processes to start and executable path must be provided.\n");
        return EXIT_FAILURE;
    }

    osmp_shared->process_count = process_count;

    // Leere Logdatei, falls vorhanden
    if (logfile_path) fclose(fopen(logfile_path, "w"));

    int log_fd = open(logfile_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (log_fd == -1) {
        perror("Could not open log file");
        return EXIT_FAILURE;
    }
    osmp_shared->log_fd = log_fd;

    // Starte Logging
    snprintf(buffer, sizeof(buffer), "Starting %d instances of %s", process_count, executable_path);
    OSMP_Log(OSMP_LOG_BIB_CALL, buffer);

    pid_t pid_children[process_count]; // Array zur Speicherung der Kinderprozess-IDs

    for (int i = 0; i < process_count; i++) {
        int rank;

        // 🔐 Rank nur im Elternprozess vergeben – und vor dem Fork!
        if (osmp_shared->free_rank_count == 0) {
            fprintf(stderr, "No free ranks available\n");
            OSMP_Log(OSMP_LOG_FAILS, "No free ranks available");
            break;
        }
        rank = osmp_shared->free_ranks[osmp_shared->front];
        osmp_shared->front = (osmp_shared->front + 1) % OSMP_MAX_PROCESSES;
        osmp_shared->free_rank_count--;

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            OSMP_Log(OSMP_LOG_FAILS, "Fork failed");
            // Rank zurück in Queue einreihen, da Prozess nicht gestartet wurde
            osmp_shared->front = (osmp_shared->front - 1 + OSMP_MAX_PROCESSES) % OSMP_MAX_PROCESSES;
            osmp_shared->free_rank_count++;
            continue;
        } else if (pid == 0) {
            // Kindprozess – führt nur exec() aus
            execvp(executable_path, exec_args);

            // Nur erreichbar, wenn exec scheitert
            perror("Exec failed");
            OSMP_Log(OSMP_LOG_FAILS, "Exec failed");
            exit(EXIT_FAILURE);
        } else {
            // ✅ Nur hier im Elternprozess die pid_map schreiben
            osmp_shared->pid_map[rank] = pid;

            snprintf(buffer, sizeof(buffer), "Started instance %d with PID %d (Rank %d)", i + 1, pid, rank);
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
            continue;
        } else{
            if (WIFEXITED(status)) { // Liest Bits des Status – sauberer Abgang, wenn WIFEXITED true ist
                // Finde den Rang des beendeten Prozesses anhand der PID
                int freed_rank = -1;
                for (int r = 0; r < OSMP_MAX_PROCESSES; r++) {
                    if (osmp_shared->pid_map[r] == pid) {
                        freed_rank = r;
                        break;
                    }
                }
                if (freed_rank != -1) {
                    osmp_shared->pid_map[freed_rank] = -1;
                    osmp_shared->free_ranks[osmp_shared->rear++] = freed_rank;
                    osmp_shared->rear %= OSMP_MAX_PROCESSES;
                    osmp_shared->free_rank_count++;
                } else{
                    snprintf(buffer, sizeof(buffer), "PID %d not found in pid_map", pid);
                    OSMP_Log(OSMP_LOG_FAILS, buffer);
                }
                int exit_status = WEXITSTATUS(status);
                snprintf(buffer, sizeof(buffer), "Child process %d exited with code %d", pid, exit_status);
                OSMP_Log(OSMP_LOG_BIB_CALL, buffer);
            } else{
                snprintf(buffer, sizeof(buffer), "Child process %d terminated abnormally", pid);
                OSMP_Log(OSMP_LOG_FAILS, buffer);
            }
        }
    }

    if (log_fd != -1) {
        close(log_fd);
    }
    return EXIT_SUCCESS;
}
