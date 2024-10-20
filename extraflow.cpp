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

struct File {
    std::string name;
};

// Hashmaps to store nodes, pipes, concatenates, and files
std::unordered_map<std::string, Node> nodes;
std::unordered_map<std::string, Pipe> pipes;
std::unordered_map<std::string, Concatenate> concatenates;
std::unordered_map<std::string, File> files;

// Forward declare the functions to resolve the circular dependency
void run_pipe(const std::string& pipe_name);
void execute_concatenate(const std::string& concat_name);

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

// Function to execute a single command, with optional file input and error handling
void execute_command(const std::string& command, const std::string* file = nullptr, bool redirect_stderr = false) {
    std::vector<char*> args = split_command(command);

    if (fork() == 0) {
        if (file) {
            // Redirect input from the file if specified
            freopen(file->c_str(), "r", stdin);
        }
        
        if (redirect_stderr) {
            // Redirect stderr to stdout for error handling
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }

        execvp(args[0], args.data());
        perror("execvp failed");
        exit(1);
    }
    wait(nullptr);  // Wait for the command to complete
}

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

        // Handle the "from" part, checking if it's a node, pipe, or concatenate
        if (nodes.find(p.from) != nodes.end()) {
            execute_command(nodes[p.from].command);
        } else if (pipes.find(p.from) != pipes.end()) {
            run_pipe(p.from);
        } else if (concatenates.find(p.from) != concatenates.end()) {
            execute_concatenate(p.from);
        } else if (files.find(p.from) != files.end()) {
            // If the "from" part is a file, redirect the file content
            execute_command("cat", &files[p.from].name);
        }
    } else {
        // Parent process handles the "to" part
        close(fd[1]);  // Close the write end (parent doesn't need to write)
        dup2(fd[0], STDIN_FILENO);  // Redirect stdin to pipe's read end
        close(fd[0]);  // Close the read end after redirection

        if (nodes.find(p.to) != nodes.end()) {
            execute_command(nodes[p.to].command);
        } else if (pipes.find(p.to) != pipes.end()) {
            run_pipe(p.to);
        } else if (concatenates.find(p.to) != concatenates.end()) {
            execute_concatenate(p.to);
        }

        // Wait for the child process to finish
        wait(nullptr);
    }
}

// Function to handle concatenations (can include nodes, pipes, or other concatenations)
void execute_concatenate(const std::string& concat_name) {
    Concatenate concat = concatenates[concat_name];

    // First, execute the first node to print its output
    if (nodes.find(concat.parts[0]) != nodes.end()) {
        if (fork() == 0) {
            // First node, print its output
            execute_command(nodes[concat.parts[0]].command);
        }
        wait(nullptr);  // Wait for the first part to finish
    }

    // Then execute the pipe (or other node/concatenate) to handle the rest
    if (pipes.find(concat.parts[1]) != pipes.end()) {
        run_pipe(concat.parts[1]);
    } else if (nodes.find(concat.parts[1]) != nodes.end()) {
        if (fork() == 0) {
            execute_command(nodes[concat.parts[1]].command);
        }
        wait(nullptr);
    } else if (concatenates.find(concat.parts[1]) != concatenates.end()) {
        execute_concatenate(concat.parts[1]);
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

// Function to parse the flow file and populate nodes, pipes, concatenates, and files
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
        } else if (line.rfind("file=", 0) == 0) {
            std::string file_name = line.substr(5);
            getline(file, line);
            std::string file_path = line.substr(5);
            files[file_name] = File{file_path};
        } else if (line.rfind("stderr=", 0) == 0) {
            std::string node_name = line.substr(7);
            getline(file, line);
            std::string from_node = line.substr(5);
            // Handle the case where we need to redirect stderr to stdout
            nodes[node_name] = Node{"true"};  // Placeholder node for handling redirection
            pipes[node_name] = Pipe{from_node, node_name};
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
