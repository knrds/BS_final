//
// Created by knrd on 02.04.25.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "osmpRun.h"
#include "../osmpLibrary/OSMP.h"
#include "../osmpLibrary/osmpLib.h"
#include <bits/getopt_core.h>

char *logfile_path = NULL; // Path to the log file
int verbosity_level = 1;  // Standard: Level 1
char buffer[256]; // Buffer for log messages

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

    // Clear log file if it exists
    if (logfile_path) fclose(fopen(logfile_path, "w"));

    // start logging
    snprintf(buffer, sizeof(buffer), "Starting %d instances of %s", process_count, executable_path);
    log_event_level(buffer, 1);

    pid_t pid_children[process_count]; // Array to store child process IDs

    for (int i = 0; i < process_count; i++) {
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
            log_event_level("Fork failed", 3);
            EXIT_FAILURE;
        } else{
            // Parent process
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
                int exit_status = WEXITSTATUS(status);
                snprintf(buffer, sizeof(buffer), "Child process %d exited with code %d", pid, exit_status);
                log_event_level(buffer, 1);
            } else{
                snprintf(buffer, sizeof(buffer), "Child process %d terminated abnormally", pid);
                log_event_level(buffer, 3);
            }
        }
    }

    return EXIT_SUCCESS;
}
