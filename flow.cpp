#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

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

std::unordered_map<std::string, Node> nodes;
std::unordered_map<std::string, Pipe> pipes;
std::unordered_map<std::string, Concatenate> concatenates;

void execute_command(const std::string& command) {
    std::vector<char*> args;
    char* cmd = strdup(command.c_str());
    char* token = strtok(cmd, " ");
    while (token != nullptr) {
        args.push_back(token);
        token = strtok(nullptr, " ");
    }
    args.push_back(nullptr);

    execvp(args[0], args.data());
    perror("execvp failed");
    exit(1);
}

void run_node(const std::string& node_name) {
    Node node = nodes[node_name];
    if (fork() == 0) {
        execute_command(node.command);
    }
    wait(nullptr);
}

void run_pipe(const std::string& pipe_name) {
    Pipe p = pipes[pipe_name];
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // First process: from node
        close(fd[0]); // Close read end
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(fd[1]);
        execute_command(nodes[p.from].command);
    }

    if (fork() == 0) {
        // Second process: to node
        close(fd[1]); // Close write end
        dup2(fd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(fd[0]);
        execute_command(nodes[p.to].command);
    }

    close(fd[0]);
    close(fd[1]);
    wait(nullptr);
    wait(nullptr);
}

void run_concatenate(const std::string& concat_name) {
    Concatenate concat = concatenates[concat_name];
    for (const std::string& part : concat.parts) {
        if (pipes.find(part) != pipes.end()) {
            run_pipe(part);
        } else {
            run_node(part);
        }
    }
}

void run_action(const std::string& action) {
    if (pipes.find(action) != pipes.end()) {
        run_pipe(action);
    } else if (concatenates.find(action) != concatenates.end()) {
        run_concatenate(action);
    } else {
        run_node(action);
    }
}

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
