#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

int timeout = -1;
int input_tcp = 0, output_tcp = 0, input_udp = 0, output_udp = 0;
int input_uds = 0, output_uds = 0;
int input_port = -1, output_port = -1; 
int input_fd = -1, output_fd = -1;
char *input_uds_path = NULL, *output_uds_path = NULL;

void close_receiver(int fd) {
    if (fd != -1) { // Check if the receiver socket fd is valid (not -1)
        close(fd); // Close the receiver socket fd
    }
}

// Signal handler for timeout
void handle_timeout(int sig) {
    (void)sig;
    printf("Timeout reached. Closing connection.\n");
    close_receiver(SIGINT); // Close the connection
    kill(0, SIGKILL); // Kill all processes in the current process group
}

void server(int port, int *input_fd, int is_udp) {
    struct sockaddr_in server_addr;

    if (is_udp) {
        printf("is_udp %d\n", is_udp);
        *input_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        printf("input_fd %ls\n", input_fd);
        *input_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (*input_fd < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(*input_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        exit(1);
    }

    if (!is_udp) {
        printf("server listening on port %d\n", port);
        if (listen(*input_fd, 1) < 0) {
            perror("Failed to listen on socket");
            exit(1);
        }
        printf("server listening on port %d\n", port);
        int client_fd = accept(*input_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Failed to accept connection");
            exit(1);
        }
        close(*input_fd);
        *input_fd = client_fd;
    }
}

void client(const char *host, int port, int *output_fd, int is_udp) {
    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(host);

    if (server == NULL) {
        fprintf(stderr, "Failed to get host\n");
        exit(1);
    }

    if (is_udp) {
        *output_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        *output_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (*output_fd < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // Use inet_pton to convert the host address string to binary format
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n");
        exit(1);
    }

    server_addr.sin_port = htons(port);

    if (connect(*output_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        exit(1);
    }
}

void uds_server(const char *path, int *input_fd, int is_stream) {
    printf("server is start\n");
    struct sockaddr_un server_addr;

    if (is_stream) {
        printf("is_stream\n");
        *input_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    } else {
        printf("is_DGRAM\n");
        *input_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    }

    if (*input_fd < 0) {
        perror("Failed to create UDS socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    unlink(path); // Remove the file if it already exists

    if (bind(*input_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind UDS socket");
        exit(1);
    }
    printf("bind.. \n");

    if (is_stream) {
        if (listen(*input_fd, 1) < 0) {
            perror("Failed to listen on UDS socket");
            exit(1);
        }
        int client_fd = accept(*input_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Failed to accept UDS connection");
            exit(1);
        }
        close(*input_fd);
        *input_fd = client_fd;
    }
}

void uds_client(const char *path, int *output_fd, int is_stream) {
    struct sockaddr_un server_addr;

    if (is_stream) {
        *output_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    } else {
        *output_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    }

    if (*output_fd < 0) {
        perror("Failed to create UDS socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    if (connect(*output_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to UDS server");
        exit(1);
    }
}

void redirect_in(int fd) {
    if (fd != -1) {
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("Failed to redirect input");
            exit(1);
        }
    }
}

void redirect_out(int fd) {
    if (fd != -1) {
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Failed to redirect output");
            exit(1);
        }
    }
}

void printErrorAndExit(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printErrorAndExit("Invalid arguments");
    }

    char *exec = NULL;
    char *output_host = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            exec = argv[++i];
        } 
        // -i flag
        else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            char *arg = argv[++i];

            // tcp server
            if (strncmp(arg, "TCPS", 4) == 0) {
                input_tcp = 1;
                input_port = atoi(arg + 4);
            } 
            // udp server    
            else if (strncmp(arg, "UDPS", 4) == 0) {
                input_udp = 1;
                input_port = atoi(arg + 4);
            } 
            // UDS Datagram server
            else if (strncmp(arg, "UDSSD", 5) == 0) {
                input_uds = 1;
                input_uds_path = arg + 5;
                printf("udssd!\n ");
                printf("input_uds_path: %s\n,",input_uds_path);
            } 
            // UDS Stream server
            else if (strncmp(arg, "UDSSS", 5) == 0) {
                input_uds = 1;
                input_uds_path = arg + 5;
            } 
            else {
                printErrorAndExit("Invalid input parameter");
            }
        } 
        // -o flag
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            char *arg = argv[++i];
            if (strncmp(arg, "TCPC", 4) == 0) {
                output_tcp = 1;
                char *command = strchr(arg, ',');
                if (command == NULL) {
                    printErrorAndExit("Invalid TCP client parameter");
                }
                *command = '\0';
                output_host = arg + 4;
                output_port = atoi(command + 1);
            } 
            else if (strncmp(arg, "UDPC", 4) == 0) {
                output_udp = 1;
                char *command = strchr(arg, ',');
                if (command == NULL) {
                    printErrorAndExit("Invalid UDP client parameter");
                }
                *command = '\0';
                output_host = arg + 4;
                output_port = atoi(command + 1);
            } 
            // UDS Datagram client
            else if (strncmp(arg, "UDSCD", 5) == 0) {
                output_uds = 1;
                output_uds_path = arg + 5;
           
            }
            // UDS Stream client
            else if (strncmp(arg, "UDSCS", 5) == 0) {
                output_uds = 1;
                output_uds_path = arg + 5;
            } 
            else {
                printErrorAndExit("Invalid output parameter");
            }
        } 
        // -b flag in tcp
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            char *arg = argv[++i];
            if (strncmp(arg, "TCPS", 4) == 0) {
                input_tcp = 1;
                output_tcp = 1;
                input_port = atoi(arg + 4);
                output_port = input_port;
            } else {
                printErrorAndExit("Invalid -b parameter");
            }
        }
        //-t flag: timer in udp
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        } 
        else {
            printErrorAndExit("Invalid parameter");
        }
    }

    if (exec == NULL) {
        printErrorAndExit("Executable not specified");
    }

    //-i flag
    if (input_tcp || input_udp) {
        server(input_port, &input_fd, input_udp);
    } else if (input_uds) {
        printf("input usd\n");
        printf("argv[4] %s: \n", argv[4]);
        if (strncmp(argv[4], "UDSSD", 5) == 0) {
            printf("stsrt uds server\n");
            uds_server(input_uds_path, &input_fd, 0); // Datagram server
        } else if (strncmp(argv[4], "UDSSS", 5) == 0) {
            uds_server(input_uds_path, &input_fd, 1); // Stream server
        }
    }

    //-o flag
    if (output_tcp || output_udp) {
        client(output_host, output_port, &output_fd, output_udp);
    } else if (output_uds) {
        if (strncmp(argv[4], "UDSCD", 5) == 0) {
            uds_client(output_uds_path, &output_fd, 0); // Datagram client
        } else if (strncmp(argv[4], "UDSCS", 5) == 0) {
            uds_client(output_uds_path, &output_fd, 1); // Stream client
        }
    }

    if (input_tcp || input_udp || input_uds) { //-i - redirect to input
        redirect_in(input_fd);
    }

    if (output_tcp || output_udp || output_uds) { //-o - redirect to output
        redirect_out(output_fd);
    }

    if (timeout > 0) {  //-t flag: timer in udp
        signal(SIGALRM, handle_timeout);
        alarm(timeout);
    }

    printf("Executing command: %s\n", exec);

    int pid = fork();
    if (pid < 0) {
        perror("Failed to fork");
        exit(1);
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", exec, NULL);
        perror("Failed to execute program");
        exit(1);
    } else {
        int status;
        wait(&status);
        if (input_fd > 0) close(input_fd);
        if (output_fd > 0) close(output_fd);
    }

    return 0;
}
