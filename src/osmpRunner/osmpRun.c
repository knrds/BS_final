//
// Created by knrd on 02.04.25.
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



char *logfile_path = NULL; // Path to the log file
int verbosity_level = 1;  // Standard: Level 1
char buffer[256]; // Buffer for log messages

osmp_shared_info_t *osmp_shared = NULL; // globaler Zeiger

int setup_shared_memory(void){
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666); // Create shared memory object

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
    if (ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    osmp_shared = (osmp_shared_info_t *)ptr;
    memset(osmp_shared, 0, SHM_SIZE); // Zero init

    // Init process map and free ranks queue
    for (int i = 0; i < OSMP_MAX_PROCESSES; i++) {
        osmp_shared->free_ranks[i] = i;
        osmp_shared->pid_map[i] = -1;
    }
    osmp_shared->front = 0;
    osmp_shared->rear = OSMP_MAX_PROCESSES;

    // 🟡 Neu: Logfile-Pfad und Verbosity speichern
    if (logfile_path != NULL) {
        strncpy(osmp_shared->logfile_path, logfile_path, sizeof(osmp_shared->logfile_path) - 1);
        osmp_shared->logfile_path[sizeof(osmp_shared->logfile_path) - 1] = '\0'; // Safety null-terminator
    } else {
        osmp_shared->logfile_path[0] = '\0'; // leer lassen
    }

    osmp_shared->verbosity_level = verbosity_level;

    close(fd); // FD kann geschlossen werden
    return 0;
}


void log_event_level (const char *message, int level) {
    if (level > verbosity_level) return;
    // Check if the log file path is set
    if (logfile_path == NULL) {
        fprintf(stderr, "Log file path is not set.\n");
        return;
    }
    // Open the log file in append mode
    FILE *log_file = fopen(logfile_path, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    // Write the message to the log file with a timestamp and PID
    time_t now = time(NULL);
    fprintf(log_file, "[%02d:%02d:%02d] PID %d: %s\n",
            localtime(&now)->tm_hour,
            localtime(&now)->tm_min,
            localtime(&now)->tm_sec,
            getpid(),
            message);

    // Close the log file
    fclose(log_file);
}



int main(int argc, char *argv[]) {
    int opt; // Variable for command line options
    int process_count = -1; // Number of processes to start
    char *executable_path = NULL; // Path to the executable
    char **exec_args = NULL; // Arguments for the executable

    if (setup_shared_memory() != 0) {
        fprintf(stderr, "Shared Memory Setup failed\n");
        return EXIT_FAILURE;
    }

    // Parse command line options
    while((opt = getopt(argc, argv, "p:l:v:e:")) != -1) {
        switch (opt) {
            // -p: Number of processes to start
            case 'p': {
                    char *endptr1;
                    process_count = (int) strtol(optarg, &endptr1, 10); // Convert string to long
                    if (*endptr1 != '\0' || process_count <= 0) {
                        fprintf(stderr, "Invalid process count: %s\n", optarg);
                        return EXIT_FAILURE;
                    }
                }
                break;
            // -l: Path to the log file
            case 'l':
                logfile_path = optarg;
                break;
            // -v: Verbosity level
            case 'v':{
                    char *endptr;
                    verbosity_level = (int) strtol(optarg, &endptr, 10);
                    if (*endptr != '\0' || verbosity_level < 1 || verbosity_level > 3) {
                        fprintf(stderr, "Invalid Log-Level: %s\n", optarg);
                        return EXIT_FAILURE;
                    }
                }
                break;
            // -e: Path to the executable
            case 'e':
                executable_path = optarg;
                exec_args = &argv[optind - 1]; // Set the executable path without "-e"
                exec_args[0] = executable_path; // Set the executable path correctly
                break;
            // Invalid option - Usage message
            default:
                fprintf(stderr, "Usage: %s [-p process_count] [-l logfile_path] [-v verbosity_level] [-e executable_path]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check if the number of processes to start and executable path are provided
    if (process_count <= 0 || executable_path == NULL) {
        fprintf(stderr, "Error: Number of processes to start and executable path must be provided.\n");
        return EXIT_FAILURE;
    }

    osmp_shared->process_count = process_count;

    // Clear log file if it exists
    if (logfile_path) fclose(fopen(logfile_path, "w"));

    int log_fd = open(logfile_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (log_fd == -1) {
        perror("Could not open log file");
        return EXIT_FAILURE;
    }
    osmp_shared->log_fd = log_fd;


    // start logging
    snprintf(buffer, sizeof(buffer), "Starting %d instances of %s", process_count, executable_path);
    log_event_level(buffer, 1);

    pid_t pid_children[process_count]; // Array to store child process IDs

    for (int i = 0; i < process_count; i++) {
        // Check if there are free ranks available
        if (osmp_shared->front == osmp_shared->rear) {
            fprintf(stderr, "No free ranks available\n");
            exit(EXIT_FAILURE);
        }
        int rank = osmp_shared->free_ranks[osmp_shared->front++];
        osmp_shared->front %= OSMP_MAX_PROCESSES;

        pid_t pid = fork();

        if (pid < 0) {
            // Fork failed
            perror("Fork failed");
            log_event_level("Fork failed", 3);
            continue;
        } else if (pid == 0) {
            // Child process
            // Execute the program with the provided arguments
            execvp(executable_path, exec_args);

            // Only reachable if execvp fails
            perror("Exec failed");
            log_event_level("Exec failed", 3);
            exit(EXIT_FAILURE);
        } else{
            // Parent process
            // Store the child process ID in the shared memory
            osmp_shared->pid_map[rank] = pid;
            snprintf(buffer, sizeof(buffer), "Started instance %d with PID %d", i + 1, pid);
            pid_children[i] = pid;
            log_event_level(buffer, 1);
        }
    }


    // Wait for all child processes to finish
    for (int i = 0; i < process_count; i++) {
        int status;
        pid_t pid = waitpid(pid_children[i], &status, 0); // Wait for the child process to finish

        if (pid == -1) {
            perror("Error: Wait failed");
            log_event_level("Wait failed", 3);
            continue;
        } else{
            if (WIFEXITED(status)) { // reads bits of status - clean exit if WIFEXITED is true
                // Finde den Rank des beendeten Prozesses anhand der PID
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
                } else{
                    snprintf(buffer, sizeof(buffer), "PID %d not found in pid_map", pid);
                    log_event_level(buffer, 3);
                }
                int exit_status = WEXITSTATUS(status);
                snprintf(buffer, sizeof(buffer), "Child process %d exited with code %d", pid, exit_status);
                log_event_level(buffer, 1);
            } else{
                snprintf(buffer, sizeof(buffer), "Child process %d terminated abnormally", pid);
                log_event_level(buffer, 3);
            }
        }
    }

    if (log_fd != -1) {
        close(log_fd);
    }
    return EXIT_SUCCESS;
}
