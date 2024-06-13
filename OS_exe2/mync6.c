#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int timeout = -1;
int input_uds = 0, output_uds = 0;
char *input_socket_path = NULL, *output_socket_path = NULL;
int input_fd = -1, output_fd = -1;

// Function to close the receiver socket
void close_receiver(int fd) {
    if (fd != -1) {
        close(fd);
    }
}

// Signal handler for timeout
void handle_timeout(int sig) {
    (void)sig;
    printf("Timeout reached. Closing connection.\n");
    close_receiver(input_fd);
    close_receiver(output_fd);
    kill(0, SIGKILL);
}

// Function to create and set up the server Unix domain socket
void server(const char *socket_path, int *input_fd) {
    struct sockaddr_un server_addr;

    *input_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*input_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    unlink(socket_path); // Remove any previous socket with the same name
    if (bind(*input_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(*input_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int client_fd = accept(*input_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    close(*input_fd);
    *input_fd = client_fd;
}

// Function to create and set up the client Unix domain socket
void client(const char *socket_path, int *output_fd) {
    struct sockaddr_un server_addr;

    *output_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*output_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (connect(*output_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

// Function to redirect input
void redirect_in(int fd) {
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
    }
}

// Function to redirect output
void redirect_out(int fd) {
    if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
    }
}

int main(int argc, char *argv[]) {
    char *exec = NULL;
    char *output_host = NULL;
    int output_port = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            exec = argv[++i];
        } 
        else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            char *arg = argv[++i];

            if (strncmp(arg, "UDSS", 4) == 0) {
                input_uds = 1;
                input_socket_path = arg + 4;
            } 
            else {
                printf("Invalid input argument\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            char *arg = argv[++i];

            if (strncmp(arg, "UDSC", 4) == 0) {
                output_uds = 1;
                output_socket_path = arg + 4;
            } 
            else {
                printf("Invalid output argument\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        } 
        else {
            printf("Invalid argument\n");
            exit(EXIT_FAILURE);
        }
    }

    if (exec == NULL) {
        printf("Executable not specified\n");
        exit(EXIT_FAILURE);
    }

    if (input_uds) {
        server(input_socket_path, &input_fd);
    }
    
    if (output_uds) {
        client(output_socket_path, &output_fd);
    }

    if (input_uds) {
        redirect_in(input_fd);
    }

    if (output_uds) {
        redirect_out(output_fd);
    }

    if (timeout > 0) {
        signal(SIGALRM, handle_timeout);
        alarm(timeout);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", exec, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } 
    else {
        int status;
        wait(&status);
        if (input_fd != -1) {
            close(input_fd);
        }
        if (output_fd != -1) {
            close(output_fd);
        }
    }

    return 0;
}
