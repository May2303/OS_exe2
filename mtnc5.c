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

int timeout = -1;
int input_tcp = 0, output_tcp = 0, input_udp = 0, output_udp = 0;
int input_port = -1, output_port = -1; 
int input_fd = -1, output_fd = -1;

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
        *input_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        *input_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (*input_fd < 0) {
        printf("Failed to create socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(*input_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to bind socket");
        exit(1);
    }

    if (!is_udp) {
        if (listen(*input_fd, 1) < 0) {
            printf("Failed to listen on socket");
            exit(1);
        }
        int client_fd = accept(*input_fd, NULL, NULL);
        if (client_fd < 0) {
            printf("Failed to accept connection");
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
        printf("Failed get host");
        exit(1);
    }

    if (is_udp) {
        *output_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        *output_fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (*output_fd < 0) {
        printf("Failed create socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // Use inet_pton to convert the host address string to binary format
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        printf("Invalid address");
        exit(1);
    }

    server_addr.sin_port = htons(port);

    if (connect(*output_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to connect to server");
        exit(1);
    }
}

void redirect_in(int fd) {
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
    }
}

void redirect_out(int fd) {
    if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Invalid arguments");
        exit(1);
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

            //tcp server
            if (strncmp(arg, "TCPS", 4) == 0) {
                input_tcp = 1;
                input_port = atoi(arg + 4);

            // udp server    
            } else if (strncmp(arg, "UDPS", 4) == 0) {
                input_udp = 1;
                input_port = atoi(arg + 4);
            } else {
                printErrorAndExit("Invalid input argeter");
            }
        }
        //-o flag
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            char *arg = argv[++i];
            if (strncmp(arg, "TCPC", 4) == 0) {
                output_tcp = 1;
                char *command = strchr(arg, ',');
                if (command == NULL) {
                    printErrorAndExit("Invalid TCP client argeter");
                }
                *command = '\0';
                output_host = arg + 4;
                output_port = atoi(command + 1);
            } 
            else if (strncmp(arg, "UDPC", 4) == 0) {
                output_udp = 1;
                char *command = strchr(arg, ',');
                if (command == NULL) {
                    printErrorAndExit("Invalid UDP client argeter");
                }
                *command = '\0';
                output_host = arg + 4;
                output_port = atoi(command + 1);
            }
            else {
                 printf("Invalid argeter");
                 exit(1);
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
                printf("Invalid argeter");
                exit(1);
            }
        //-t flag: timer in udp
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
               timeout = atoi(argv[++i]);
        } 
        else {
            printf("Invalid argeter");
                 exit(1);
        }
    }

    if (exec == NULL) {
        printErrorAndExit("Executable not specified");
    }

    //-i flag
    if (input_tcp || input_udp) {
        server(input_port, &input_fd, input_udp);
    }
    
    //-o flag
    if (output_tcp || output_udp) {
        client(output_host, output_port, &output_fd, output_udp);
    }

    if (input_tcp || input_udp) { //-i - direct to input
        redirect_in(input_fd);
    }

    if (output_tcp || output_udp) { //-o - direct to output
        redirect_out(output_fd);
    }

    if (timeout > 0) {  //if -t flag is on 
        signal(SIGALRM, handle_timeout);
        alarm(timeout);
    }

    int pid = fork();
    if (pid < 0) {
        printf("Invalid fork");
        exit(1);
    } 
    else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", exec, NULL);
        printf("Failed to execute program");
        exit(1);
    } 
    else {
        int status;
        wait(&status);
        if (input_fd > 0) close(input_fd);
        if (output_fd > 0) close(output_fd);
    }

    return 0;
}
