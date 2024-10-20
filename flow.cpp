#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

// Data structures for Nodes, Pipes, and Concatenates
struct Node {
    std::string command;
};

struct Pipe {
    std::string from;
    std::string to;
};

struct Concatenate {
    std::vector<std::string> parts;
};

// Hashmaps to store nodes, pipes, and concatenates
std::unordered_map<std::string, Node> nodes;
std::unordered_map<std::string, Pipe> pipes;
std::unordered_map<std::string, Concatenate> concatenates;

// Function to remove quotes from the beginning and end of a string, if they exist
std::string remove_quotes(const std::string& str) {
    if (str.length() >= 2 && 
        ((str.front() == '\'' && str.back() == '\'') || (str.front() == '"' && str.back() == '"'))) {
        return str.substr(1, str.length() - 2);  // Remove the first and last character
    }
    return str;
}

// Function to split commands into arguments while respecting quotes
std::vector<char*> split_command(const std::string& command) {
    std::vector<char*> args;
    std::stringstream ss(command);
    std::string token;
    bool inside_quotes = false;
    std::string quoted_token;

    while (ss >> token) {
        if (token.front() == '\'' || token.front() == '"') {
            inside_quotes = true;
            quoted_token = token;

            if (token.back() == '\'' || token.back() == '"') {
                inside_quotes = false;
                args.push_back(strdup(remove_quotes(quoted_token).c_str()));
            }
        } else if (inside_quotes) {
            quoted_token += " " + token;
            if (token.back() == '\'' || token.back() == '"') {
                inside_quotes = false;
                args.push_back(strdup(remove_quotes(quoted_token).c_str()));
            }
        } else {
            args.push_back(strdup(remove_quotes(token).c_str()));
        }
    }
    args.push_back(nullptr); // Null terminate the argument list for execvp
    return args;
}

// Function to execute a single command
void execute_command(const std::string& command) {
    std::vector<char*> args = split_command(command);
    execvp(args[0], args.data());
    perror("execvp failed");
    exit(1);
}

// Forward declare the functions to resolve the circular dependency
void run_pipe(const std::string& pipe_name);
void execute_concatenate(const std::string& concat_name);

// Function to handle a pipe between two actions (could be a node, another pipe, or concatenation)
void run_pipe(const std::string& pipe_name) {
    Pipe p = pipes[pipe_name];
    int fd[2];
    pipe(fd);  // Create a pipe (fd[0] for reading, fd[1] for writing)

    if (fork() == 0) {
        // Child process handles the "from" part
        close(fd[0]);  // Close the read end (child doesn't need to read)
        dup2(fd[1], STDOUT_FILENO);  // Redirect stdout to pipe's write end
        close(fd[1]);  // Close the write end after redirection

        // The "from" part could be a node, pipe, or concatenation
        if (nodes.find(p.from) != nodes.end()) {
            execute_command(nodes[p.from].command);  // Handle node
        } else if (pipes.find(p.from) != pipes.end()) {
            run_pipe(p.from);  // Handle pipe recursively
        } else if (concatenates.find(p.from) != concatenates.end()) {
            execute_concatenate(p.from);  // Handle concatenation
        }
    } else {
        // Parent process handles the "to" part
        close(fd[1]);  // Close the write end (parent doesn't need to write)
        dup2(fd[0], STDIN_FILENO);  // Redirect stdin to pipe's read end
        close(fd[0]);  // Close the read end after redirection

        // The "to" part could be a node, pipe, or concatenation
        if (nodes.find(p.to) != nodes.end()) {
            execute_command(nodes[p.to].command);  // Handle node
        } else if (pipes.find(p.to) != pipes.end()) {
            run_pipe(p.to);  // Handle pipe recursively
        } else if (concatenates.find(p.to) != concatenates.end()) {
            execute_concatenate(p.to);  // Handle concatenation
        }

        // Wait for the child process to finish
        wait(nullptr);
    }
}

// Function to handle concatenations (can include nodes, pipes, or other concatenations)
// Updated function to handle concatenations (can include nodes, pipes, or other concatenations)
// Updated function to handle concatenations (can include nodes, pipes, or other concatenations)
void execute_concatenate(const std::string& concat_name) {
    Concatenate concat = concatenates[concat_name];

    for (const std::string& part : concat.parts) {
        if (nodes.find(part) != nodes.end()) {
            // If the part is a node, directly execute the command
            if (fork() == 0) {
                execute_command(nodes[part].command);
            }
            wait(nullptr);  // Wait for the node to finish execution
        } else if (pipes.find(part) != pipes.end()) {
            // If the part is a pipe, call run_pipe to handle it
            run_pipe(part);
        } else if (concatenates.find(part) != concatenates.end()) {
            // If the part is another concatenate, call execute_concatenate recursively
            execute_concatenate(part);
        }
    }
}



// Function to run a single node (command)
void run_node(const std::string& node_name) {
    Node node = nodes[node_name];
    if (fork() == 0) {
        execute_command(node.command);
    }
    wait(nullptr);
}

// Function to run the specified action (could be a node, pipe, or concatenation)
void run_action(const std::string& action) {
    if (nodes.find(action) != nodes.end()) {
        run_node(action);
    } else if (pipes.find(action) != pipes.end()) {
        run_pipe(action);
    } else if (concatenates.find(action) != concatenates.end()) {
        execute_concatenate(action);
    }
}

// Function to parse the flow file and populate nodes, pipes, and concatenates
void parse_flow_file(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    
    while (getline(file, line)) {
        if (line.rfind("node=", 0) == 0) {
            std::string node_name = line.substr(5);
            getline(file, line);
            std::string command = line.substr(8);
            nodes[node_name] = Node{command};
        } else if (line.rfind("pipe=", 0) == 0) {
            std::string pipe_name = line.substr(5);
            getline(file, line);
            std::string from_node = line.substr(5);
            getline(file, line);
            std::string to_node = line.substr(3);
            pipes[pipe_name] = Pipe{from_node, to_node};
        } else if (line.rfind("concatenate=", 0) == 0) {
            std::string concat_name = line.substr(12);
            getline(file, line);
            int parts_count = std::stoi(line.substr(6));
            Concatenate concat;
            for (int i = 0; i < parts_count; ++i) {
                getline(file, line);
                concat.parts.push_back(line.substr(7));  // "part_X="
            }
            concatenates[concat_name] = concat;
        }
    }
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./flow <flow_file> <action>" << std::endl;
        return 1;
    }

    std::string flow_file = argv[1];
    std::string action = argv[2];

    parse_flow_file(flow_file);
    run_action(action);

    return 0;
}
