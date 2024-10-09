#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_NODES 10
#define BUFFER_SIZE 1024

// Structure for holding node and pipe information
typedef struct {
    char *name;
    char *command;
} Node;

typedef struct {
    char *from;
    char *to;
} Pipe;

Node nodes[MAX_NODES];
Pipe pipes[MAX_NODES];
int node_count = 0;
int pipe_count = 0;

// Function to parse .flow file
void parse_flow_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    char line[BUFFER_SIZE];
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "node=", 5) == 0) {
            nodes[node_count].name = strdup(strtok(line + 5, "\n"));
            fgets(line, sizeof(line), file);
            nodes[node_count].command = strdup(strtok(line + 8, "\n")); // skip 'command='
            node_count++;
        } else if (strncmp(line, "pipe=", 5) == 0) {
            pipes[pipe_count].from = strdup(strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            pipes[pipe_count].to = strdup(strtok(NULL, "\n"));
            pipe_count++;
        }
    }
    fclose(file);
}

// Function to run a single command
void run_command(char *command) {
    char *args[] = {"/bin/sh", "-c", command, NULL};
    execvp(args[0], args);
    perror("execvp failed");
    exit(EXIT_FAILURE);
}

// Function to execute a pipe between two commands
void run_pipe(char *from_command, char *to_command) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Child 1: runs `from_command`
        close(fd[0]); // Close read end
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(fd[1]); // Close write end
        run_command(from_command);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // Child 2: runs `to_command`
        close(fd[1]); // Close write end
        dup2(fd[0], STDIN_FILENO); // Redirect stdin to pipe
        close(fd[0]); // Close read end
        run_command(to_command);
    }

    // Parent process: close pipes and wait for children to finish
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <flow_file> <action>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *flow_file = argv[1];
    const char *action = argv[2];

    parse_flow_file(flow_file);

    for (int i = 0; i < pipe_count; i++) {
        if (strcmp(pipes[i].from, action) == 0) {
            for (int j = 0; j < node_count; j++) {
                if (strcmp(nodes[j].name, pipes[i].from) == 0) {
                    for (int k = 0; k < node_count; k++) {
                        if (strcmp(nodes[k].name, pipes[i].to) == 0) {
                            run_pipe(nodes[j].command, nodes[k].command);
                            return 0;
                        }
                    }
                }
            }
        }
    }

    fprintf(stderr, "No matching action found in the flow file.\n");
    return EXIT_FAILURE;
}
