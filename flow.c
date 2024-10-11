#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#define TABLE_SIZE 10
#define BUFFER_SIZE 1024

typedef struct Node {
    char *name;
    char *command;
    struct Node *next;
} Node;

typedef struct Pipe{
    char *name;
    char *from; 
    char *to;
    struct Pipe *next;
} Pipe;


Node *node_table[TABLE_SIZE];
Pipe *pipe_table[TABLE_SIZE];

// Simple hash function
unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash << 5) + *key++;
    }
    return hash % TABLE_SIZE;
}

// Add a node to the hashmap
void add_node(const char *name, const char *command) {
    unsigned int index = hash(name);
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->name = strdup(name);
    new_node->command = strdup(command);
    new_node->next = node_table[index];
    node_table[index] = new_node;
}

// Add a pipe to the hashmap
void add_pipe(const char *name, const char *from, const char *to) {
    unsigned int index = hash(name); 
    Pipe *new_pipe = (Pipe *)malloc(sizeof(Pipe));
    new_pipe->name = strdup(name);
    new_pipe->from = strdup(from);
    new_pipe->to = strdup(to);
    new_pipe->next = pipe_table[index];
    pipe_table[index] = new_pipe;

    printf("Added pipe: name=%s, from=%s, to=%s\n", name, from, to);
}



// Lookup a node in the hashmap
Node *find_node(const char *name) {
    unsigned int index = hash(name);
    Node *node = node_table[index];
    while (node) {
        if (strcmp(node->name, name) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

// Lookup a pipe in the hashmap
Pipe *find_pipe(const char *name) {
    unsigned int index = hash(name); 
    Pipe *pipe = pipe_table[index];
    while (pipe) {
        if (strcmp(pipe->name, name) == 0) {
            return pipe;
        }
        pipe = pipe->next;
    }
    return NULL; 
}


// Function to parse the .flow file and populate the hashmaps
void parse_flow_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "node=", 5) == 0) {
            char *name = strdup(strtok(line + 5, "\n"));
            fgets(line, sizeof(line), file);
            char *command = strdup(strtok(line + 8, "\n"));
            add_node(name, command);
        } 
        else if (strncmp(line, "pipe=", 5) == 0) {
            char *pipe_name = strdup(strtok(line + 5, "\n")); 
            fgets(line, sizeof(line), file); 
            char *from = strdup(strtok(line + 5, "\n"));
            fgets(line, sizeof(line), file);
            char *to = strdup(strtok(line + 3, "\n"));
            add_pipe(pipe_name, from, to);
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
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        run_command(from_command);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO); 
        close(fd[0]);
        run_command(to_command);
    }

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

    Pipe *pipe = find_pipe(action);
    if (pipe) {
        printf("Found pipe: name=%s, from=%s, to=%s\n", pipe->name, pipe->from, pipe->to); 

        Node *from_node = find_node(pipe->from);
        Node *to_node = find_node(pipe->to);

        if (from_node && to_node) {
            printf("Running pipe: %s | %s\n", from_node->command, to_node->command);
            run_pipe(from_node->command, to_node->command);
        } else {
            fprintf(stderr, "Invalid nodes for the pipe\n");
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "No matching action found in the flow file.\n");
        return EXIT_FAILURE;
    }

    return 0;
}
