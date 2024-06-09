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
#include <signal.h>

int kill(pid_t pid, int sig); //CHECK FOR ME 

int sender_fd= -1;
int receiver_fd = -1; // Global variable to receiver socket file descriptor
int timeout = 0; // Global variable to timeout

void close_receiver(int sig) {
    (void)sig; //unused parameter warning
    if (receiver_fd != -1) { // Check if the receiver socket fd is valid (not -1)
        close(receiver_fd); // Close the receiver socket fd
        kill(0, SIGKILL); // Kill all processes in the current process group
    }
}

// Signal handler for timeout
void handle_timeout(int sig) {
    (void)sig;
    printf("Timeout reached. Closing connection.\n");
    close_receiver(SIGINT); // Close the connection
    kill(0, SIGKILL); // Kill all processes in the current process group
}

int create_udp_server(int port) {
    int sockfd;
    struct sockaddr_in myaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    printf("UDP server socket created on port %d\n", port);

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
     printf("UDP server bind\n");

    receiver_fd = sockfd; // Assigning the receiver socket file descriptor
    printf("the fd is: %d\n",receiver_fd);
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
    printf("UDP client socket created on port %d\n", port);

    theiraddr.sin_family = AF_INET;
    theiraddr.sin_port = htons(port);
    theiraddr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    memset(&(theiraddr.sin_zero), '\0', 8);

    printf("Attempting to connect to server at %s:%d\n", inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), port);

    if (connect(sockfd, (struct sockaddr *)&theiraddr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    printf("UDP client connected to server\n");
    return sockfd;
}
/*
redirect the standard input (stdin) and standard output (stdout)
of the current process to different file descriptors,
specified by new_stdin and new_stdout.
*/
void redirect_io(int new_stdin, int new_stdout) {
    if (new_stdin != -1) {
        //duplicates the file descriptor new_stdin onto the file descriptor for stdin using dup2
        dup2(new_stdin, STDIN_FILENO);
        close(new_stdin);
    }
    if (new_stdout != -1) {
        //duplicates the file descriptor new_stdout onto the file descriptor for stdout using dup2
        dup2(new_stdout, STDOUT_FILENO);
        close(new_stdout);
    }
}

void parse_argument(const char *arg, char *host, int *port) {
    // Skip the "UDPC" prefix
    arg += 4;

    // Find the position of the comma
    const char *comma = strchr(arg, ',');
    
    // If no comma is found, return an error or handle it accordingly
    if (comma == NULL) {
        fprintf(stderr, "Invalid UDP argument format: %s\n", arg);
        exit(EXIT_FAILURE);
    }
    
    // Copy the host part of the argument
    strncpy(host, arg, comma - arg);
    host[comma - arg] = '\0'; // Null-terminate the string
    
    // Parse the port part of the argument
    *port = atoi(comma + 1);
    printf("Debug: Host = %s, Port = %d\n", host, *port); // Debug print
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
         printf("Usage: %s -e \"<program> <arguments>\" [-i TCPS<port>|UDPS<port>] [-o TCPC<ip,port>|UDPC<ip,port>] [-b TCPS<port>] [-t timeout]\n", argv[0]);
        return 1;
    }

    printf("Number of arguments: %d\n", argc);
    printf("Arguments:\n");


    char *exec_command = argv[2];

    signal(SIGCHLD, close_receiver);
    
    // Go over the args we get to find the relevant flags. 
        
        // -i flag
       if (strcmp(argv[3], "-i") == 0){
            printf("argv[3]-flag: %s\n", argv[3]);
            printf("argv[4]-port server: %s\n", (argv[4]));

          //  if(strncmp(argv[4], "UDPS",4) == 0) {
                //int port = atoi(argv[4] + 4);
                int port = 4050;
                printf("the port of the server: %d\n", port);
                receiver_fd = create_udp_server(port);
                
          //  }

            // -t flag
           if (strcmp(argv[5], "-t") == 0) {
                timeout = atoi(argv[6])*6;
                printf("Timeout is %d seconds\n", timeout);

                if (timeout > 0) {
                    alarm(timeout);
                    signal(SIGALRM, handle_timeout); // Change handle_alarm to handle_timeout
                }
           }
        }   
        
        // -o flag    
        else if (strcmp(argv[3], "-o") == 0 ){
            char *arg = argv[4];
            const char *prefix = "UDPC";
            printf("argv[3]-flag type: %s\n", argv[3]);

         //   if(strncmp(arg, prefix, 4) == 0){ 
                char host[256];
                int port;
                parse_argument(argv[4] , host, &port);
                sender_fd = create_udp_client(host, port); 
              // printf("new_stdout is: %d\n", new_stdout);
              //  printf("reciver_fd is: %d\n", receiver_fd);
         //   }
    }


   // Split the second argument into program name and arguments
    char *program = strtok(argv[2], " ");
    char *arguments = strtok(NULL, "");
    int move;
    char *new_argv[] = {program, arguments, NULL};
 
// Fork to execute the game program in the child process
    pid_t pid = fork();
    if (pid < 0) {
        // Fork error
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        // Child process: Execute the game program

        if (receiver_fd != -1) {
        //duplicates the file descriptor new_stdin onto the file descriptor for stdin using dup2
           dup2(receiver_fd, STDIN_FILENO);
           close(receiver_fd);
        }
        if (receiver_fd != -1) {
        //duplicates the file descriptor new_stdout onto the file descriptor for stdout using dup2
            dup2(receiver_fd, STDOUT_FILENO);
            close(receiver_fd);
        
        }
        
        execvp(program, new_argv);
        perror("execvp");
        return 1;
    
} else {
    // Parent process: Wait for client input
    int status;
    wait(&status);
    if (receiver_fd > 0) close(receiver_fd);
    if (sender_fd > 0) close(sender_fd);
}

return 0;
}