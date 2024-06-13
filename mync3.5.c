#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

char *strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

void handle_communication(int sockfd) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t n;

    while (1) {
        // Read from stdin and send to socket
        n = read(STDIN_FILENO, buffer, MAX_BUFFER_SIZE);
        if (n <= 0) break;
        printf("Sent to receiver: %.*s\n", (int)n, buffer); // Log sent data
        write(sockfd, buffer, n);

        // Read from socket and send to stdout
        n = read(sockfd, buffer, MAX_BUFFER_SIZE);
        if (n <= 0) break;
        printf("Received from receiver: %.*s\n", (int)n, buffer); // Log received data
        write(STDOUT_FILENO, buffer, n);
    }

    if (n < 0) {
        perror("Error during communication");
    }

    close(sockfd);
}


void start_tcp_server(int port, int redirect_stdin, int redirect_stdout, int run_communication) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Socket created\n");

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Socket bound to port %d\n", port);

    if (listen(sockfd, 5) < 0) {
        perror("Error on listen");
        exit(EXIT_FAILURE);
    }
    printf("TCP server listening on port %d\n", port);

    clilen = sizeof(cli_addr);
    printf("DEBUG: Waiting for incoming connection...\n");
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("Error on accept");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Accepted incoming connection\n");

    if (run_communication) {
        handle_communication(newsockfd);
    }

    if (redirect_stdin) {
        printf("DEBUG: Redirecting stdin\n");
        if (dup2(newsockfd, STDIN_FILENO) < 0) {
            perror("Error redirecting stdin");
            exit(EXIT_FAILURE);
        }
    }
    if (redirect_stdout) {
        printf("DEBUG: Redirecting stdout\n");
        if (dup2(newsockfd, STDOUT_FILENO) < 0) {
            perror("Error redirecting stdout");
            exit(EXIT_FAILURE);
        }
    }


    close(sockfd);
    close(newsockfd);
}

void start_tcp_client(char *host, int port, int run_communication) {
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    if (run_communication) {
        handle_communication(client_socket);
    }

    if (dup2(client_socket, STDOUT_FILENO) < 0) {
        perror("Error redirecting stdout");
        exit(EXIT_FAILURE);
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {
    char *program = NULL;
    int input_port = 0, output_port = 0;
    char *output_host = NULL;
    int redirect_stdin = 0, redirect_stdout = 0;
    int is_server = 0, is_client = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            program = strdup(argv[++i]);
            printf("The program is: %s\n", program);

        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            printf("The argv[i+1] is: %s\n", argv[i + 1]);
            if (strncmp(argv[i + 1], "TCPS", 4) == 0) {
                is_server = 1;
                printf("The server is: %d\n", is_server);
                input_port = atoi(argv[i + 1] + 4);
                printf("The port is: %d\n", input_port);
            }
            if (strncmp(argv[i + 1], "TCPC", 4) == 0) {
                is_client = 1;
                // Assume localhost if not specified
                output_host = "127.0.0.1";
                output_port = atoi(argv[i + 1] + 4);
                printf("The host is: %s, the port is: %d\n", output_host, output_port);
            }
            redirect_stdin = 1;

        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            if (strncmp(argv[i + 1], "TCPS", 4) == 0) {
                is_server = 1;
                input_port = atoi(argv[i + 1] + 4);
                printf("The port is: %d\n", input_port);
            }
            if (strncmp(argv[i + 1], "TCPC", 4) == 0) {
                is_client = 1;
                // Assume localhost if not specified
                output_host = "127.0.0.1";
                output_port = atoi(argv[i + 1] + 4);
                printf("The host is: %s, the port is: %d\n", output_host, output_port);
            }
            redirect_stdout = 1;

        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            if (strncmp(argv[i + 1], "TCPS", 4) == 0) {
                is_server = 1;
                input_port = atoi(argv[i + 1] + 4);
                printf("The port is: %d\n", input_port);
            }
            if (strncmp(argv[i + 1], "TCPC", 4) == 0) {
                is_client = 1;
                // Assume localhost if not specified
                output_host = "127.0.0.1";
                output_port = atoi(argv[i + 1] + 4);
                printf("The host is: %s, the port is: %d\n", output_host, output_port);
            }
            redirect_stdout = 1;
            redirect_stdin = 1;
        }
    }
    printf("It is OK\n");

    if (program == NULL) {
        if (input_port != 0 && is_server) {
            printf("Starting TCP server on port %d\n", input_port);
            start_tcp_server(input_port, redirect_stdin, redirect_stdout, 1);
        } else if (output_host != NULL && output_port != 0 && is_client) {
            printf("Connecting to TCP server at %s:%d\n", output_host, output_port);
            start_tcp_client(output_host, output_port, 1);
        } else {
            handle_communication(STDIN_FILENO);
        }
    } else {
        if (input_port != 0 && is_server) {
            printf("Starting TCP server on port %d\n", input_port);
            start_tcp_server(input_port, redirect_stdin, redirect_stdout, 0);
        }
        if (output_host != NULL && output_port != 0 && is_client) {
            printf("Connecting to TCP server at %s:%d\n", output_host, output_port);
            start_tcp_client(output_host, output_port, 0);
        }

        if (program != NULL) {
            printf("Running external program: %s\n", program);
            system(program);
        }
    }

    return EXIT_SUCCESS;
}

