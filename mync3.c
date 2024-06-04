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


void start_tcp_server(int port) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Socket created\n");

    // Set socket options to allow reuse of address and port
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Socket bound to port %d\n", port);

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        perror("Error on listen");
        exit(EXIT_FAILURE);
    }
    printf("TCP server listening on port %d\n", port);

    // Accept incoming connection
    clilen = sizeof(cli_addr);
    printf("DEBUG: Waiting for incoming connection...\n");
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("Error on accept");
        exit(EXIT_FAILURE);
    }
    printf("DEBUG: Accepted incoming connection\n");

    // Redirect stdin and stdout to the socket
    printf("DEBUG: Redirecting stdin and stdout\n");
    if (dup2(newsockfd, STDIN_FILENO) < 0) {
        perror("Error redirecting stdin");
        exit(EXIT_FAILURE);
    }
    if (dup2(newsockfd, STDOUT_FILENO) < 0) {
        perror("Error redirecting stdout");
        exit(EXIT_FAILURE);
    }

    // Close the original socket
    close(sockfd);

    // Execute the specified program
    system("./ttt 123456789");

    // Close the new socket
    close(newsockfd);
     // Exit the server
    exit(EXIT_SUCCESS);
}




void start_tcp_client(char *host, int port) {
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

    dup2(client_socket, STDOUT_FILENO);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    char *program = NULL;
    int input_port = 0, output_port = 0;
    char *output_host = NULL;

    char *param = NULL; // Declare param outside the loop

    // Loop through command-line arguments
    for (int i = 1; i < argc; i++) {
        // Check for option flags
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            program = argv[++i];
        } 
        else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_port = atoi(argv[++i] + 4);
        } 
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_port = atoi(argv[++i] + 4);
        }
                else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            printf("DEBUG: Input argument: %s\n", argv[i]);
            char *param = strdup(argv[++i]); // Make a copy of the string
            printf("DEBUG: After strdup, param = %s\n", param);
            if (param) {
                char *port_str = strchr(param, 'S'); // Find the position of 'S' in the parameter string
                if (port_str == NULL) {
                    fprintf(stderr, "Error: Invalid -b option format.\n");
                    free(param);
                    return EXIT_FAILURE;
                }
                printf("DEBUG: After strchr, port_str = %s\n", port_str);
                port_str++; // Move past 'S' character
                printf("DEBUG: After moving past 'S', port_str = %s\n", port_str);
                char *endptr;
                input_port = strtol(port_str, &endptr, 10); // Parse the port number
                printf("DEBUG: After strtol, input_port = %d\n", input_port);
                if (*endptr != '\0') {
                    fprintf(stderr, "Error: Invalid port number.\n");
                    free(param);
                    return EXIT_FAILURE;
                }
                printf("DEBUG: Before free, param = %s\n", param);
                free(param); // Free the allocated memory
                printf("DEBUG: After free, param = %s\n", param); // This line might cause issues due to accessing freed memory
            }
        }


    }

    // Free the allocated memory if param is not NULL
    if (param != NULL) {
        free(param);
    }

    

    // Check if program option is provided
    if (program == NULL) {
        fprintf(stderr, "Error: Program argument is missing.\n");
        return EXIT_FAILURE;
    }

    // Handle input and output redirection based on options
    if (input_port != 0) {
        start_tcp_server(input_port);
    }

    if (output_host != NULL && output_port != 0) {
        start_tcp_client(output_host, output_port);
    }

    // Execute the specified program
    system(program);

    return EXIT_SUCCESS;
}