#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

// Add signal.h for kill declaration
#include <signal.h>

int receiver_fd = -1; // Global variable to store receiver socket file descriptor
int timeout = 0; // Global variable to store the timeout

void close_receiver(int sig) {
    (void)sig;
    if (receiver_fd != -1) {
        close(receiver_fd);
        receiver_fd = -1;
    }
}
int kill(pid_t pid, int sig);

void handle_alarm(int sig) {
    (void)sig;
    kill(0, SIGKILL); // Kill all processes in the current process group
}

void print_usage(char *program_name) {
    printf("Usage: %s -e \"<program> <arguments>\" [-i TCPS<port>|UDPS<port>] [-o TCPC<ip,port>|UDPC<ip,port>] [-b TCPS<port>] [-t timeout]\n", program_name);
}

int create_tcp_client(char *hostname, int port) {
    struct sockaddr_in theiraddr;
    struct hostent *he;
    int sockfd;

    if ((he = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    theiraddr.sin_family = AF_INET;
    theiraddr.sin_port = htons(port);
    theiraddr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    memset(&(theiraddr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&theiraddr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    return sockfd;
}

int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in myaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    receiver_fd = sockfd; // Assigning the receiver socket file descriptor
    return sockfd;
}

int accept_tcp_connection(int sockfd) {
    struct sockaddr_in theiraddr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int new_fd = accept(sockfd, (struct sockaddr *)&theiraddr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        exit(1);
    }
    return new_fd;
}

int create_udp_server(int port) {
    int sockfd;
    struct sockaddr_in myaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    receiver_fd = sockfd; // Assigning the receiver socket file descriptor
    return sockfd;
}

int create_udp_client(char *hostname, int port) {
    struct sockaddr_in theiraddr;
    struct hostent *he;
    int sockfd;

    if ((he = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    theiraddr.sin_family = AF_INET;
    theiraddr.sin_port = htons(port);
    theiraddr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    memset(&(theiraddr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&theiraddr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    return sockfd;
}

void redirect_io(int new_stdin, int new_stdout) {
    if (new_stdin != -1) {
        dup2(new_stdin, STDIN_FILENO);
        close(new_stdin);
    }
    if (new_stdout != -1) {
        dup2(new_stdout, STDOUT_FILENO);
        close(new_stdout);
    }
}

void parse_tcp_argument(char *arg, char *host, int *port) {
    char *colon = strchr(arg, ',');
    if (colon == NULL) {
        fprintf(stderr, "Invalid TCP argument format. Expected <ip,port> or <hostname,port>\n");
        exit(1);
    }

    *colon = '\0';
    strcpy(host, arg);
    *port = atoi(colon + 1);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
        print_usage(argv[0]);
        return 1;
    }

    printf("Number of arguments: %d\n", argc);
    printf("Arguments:\n");

    char *exec_command = argv[2];
    int new_stdin = -1, new_stdout = -1, i;
    signal(SIGCHLD, close_receiver);

    for (i = 3; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);

        if (strncmp(argv[i], "-i", 2) == 0 && strncmp(argv[i] + 2, "TCPS", 4) == 0) {
            int port = atoi(argv[i] + 6);
            int server_fd = create_tcp_server(port);
            new_stdin = accept_tcp_connection(server_fd);
            close(server_fd);
        } else if (strncmp(argv[i], "-o", 2) == 0 && strncmp(argv[i] + 2, "TCPC", 4) == 0) {
            char host[256];
            int port;
            parse_tcp_argument(argv[i] + 6, host, &port);
            new_stdout = create_tcp_client(host, port);
        } else if (strncmp(argv[i], "-b", 2) == 0 && strncmp(argv[i] + 2, "TCPS", 4) == 0) {
            int port = atoi(argv[i] + 6);
            int server_fd = create_tcp_server(port);
            new_stdin = new_stdout = accept_tcp_connection(server_fd);
            close(server_fd);
        } else if (strncmp(argv[i], "-i", 2) == 0){
            printf("argv[%d]: %s\n", i+1, (argv[i]+2));
            if(strncmp(argv[i+1], "UDPS", 4) == 0) {
            int port = atoi(argv[i] + 6);
            new_stdin = create_udp_server(port);
            printf("shirel ");
        } else if (strncmp(argv[i], "-o", 2) == 0 && strncmp(argv[i] + 2, "UDPC", 4) == 0) {
            char host[256];
            int port;
            parse_tcp_argument(argv[i] + 6, host, &port);
            new_stdout = create_udp_client(host, port);
        } else if (strncmp(argv[i], "-t", 2) == 0) {
            timeout = atoi(argv[i] + 2);
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (timeout > 0) {
        signal(SIGALRM, handle_alarm);
        alarm(timeout);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        redirect_io(new_stdin, new_stdout);
        char *args[100];
        char *token = strtok(exec_command, " ");
        int j = 0;

        while (token != NULL) {
            args[j++] = token;
            token = strtok(NULL, " ");
        }
        args[j] = NULL;

        execvp(args[0], args);
        perror("execvp");
        return 1;
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (receiver_fd != -1) {
            close(receiver_fd);
        }
    }

    return 0;
}
}