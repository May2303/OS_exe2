#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

int sender_fd = -1;
int receiver_fd = -1;
int timeout = 0;

void close_receiver(int sig) {
    (void)sig;
    if (receiver_fd != -1) {
        close(receiver_fd);
    }
}

void handle_timeout(int sig) {
    (void)sig; //unused parameter warning
    if (receiver_fd != -1) { // Check if the receiver socket fd is valid (not -1)
        close(receiver_fd); // Close the receiver socket fd
        kill(0, SIGKILL); // Kill all processes in the current process group
    }
}

int create_unix_server(char *path, int type) {
    int sockfd;
    struct sockaddr_un addr;

    if ((sockfd = socket(AF_UNIX, type, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    receiver_fd = sockfd;
    printf("Unix domain server socket created at path: %s\n", path);
    printf("Unix domain server bound to path: %s\n", path);

    return sockfd;
}

int create_unix_client(char *path, int type) {
    int sockfd;
    struct sockaddr_un addr;

    if ((sockfd = socket(AF_UNIX, type, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Unix domain client socket connected to server at path: %s\n", path);
    return sockfd;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
        printf("Usage: %s -e \"<program> <arguments>\" [-i UDSSD<path>] [-o UDSCD<path>] [-t timeout]\n", argv[0]);
        return 1;
    }

    printf("Number of arguments: %d\n", argc);
    printf("Arguments:\n");

    char *exec_command = argv[2];
    char *path = NULL;

    signal(SIGCHLD, handle_timeout);

    if (argc >= 5) {
        for (int i = 3; i < argc; i += 2) {
            if (strcmp(argv[i], "-i") == 0) {
                path = argv[i + 1] + 5;
                receiver_fd = create_unix_server(path, SOCK_DGRAM);
                printf("Unix domain server datagram socket created at path: %s\n", path);

                if (strcmp(argv[i], "-t") == 0) {
                    timeout = atoi(argv[i + 1]);
                    printf("Timeout is %d seconds\n", timeout);
                }
            
            } else if (strcmp(argv[i], "-o") == 0) {
                path = argv[i + 1] + 5;
                sender_fd = create_unix_client(path, SOCK_DGRAM);
                printf("Unix domain client datagram socket connected to server at path: %s\n", path);
            
            }
        }
    }

    if (argc >= 7) {
        for (int i = 3; i < argc; i += 2) {
            if (strcmp(argv[i], "-i") == 0) {
                path = argv[i + 1] + 5;
                receiver_fd = create_unix_server(path, SOCK_STREAM);
                printf("Unix domain server stream socket created at path: %s\n", path);
            } else if (strcmp(argv[i], "-o") == 0) {
                path = argv[i + 1] + 5;
                sender_fd = create_unix_client(path, SOCK_STREAM);
                printf("Unix domain client stream socket connected to server at path: %s\n", path);
            } else if (strcmp(argv[i], "-t") == 0) {
                timeout = atoi(argv[i + 1]);
                printf("Timeout is %d seconds\n", timeout);
            }
        }
    }

    char *program = strtok(argv[2], " ");
    char *arguments = strtok(NULL, "");
    char *new_argv[] = {program, arguments, NULL};

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        if (receiver_fd != -1) {
            dup2(receiver_fd, STDIN_FILENO);
            close_receiver(receiver_fd);
        }
        if (sender_fd != -1) {
            dup2(sender_fd, STDOUT_FILENO);
            close_receiver(sender_fd);
        }

        execvp(program, new_argv);
        perror("execvp");
        return 1;
    } else {
        int status;
        if (timeout > 0) {
            alarm(timeout);
            signal(SIGALRM, handle_timeout);
        }
        wait(&status);
        if (receiver_fd > 0)
            close_receiver(receiver_fd);
        if (sender_fd > 0)
            close_receiver(sender_fd);
    }

    return 0;
}
