#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void print_error() {
    fprintf(stderr, "Usage: mync -e \"program\"\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-e") != 0) {
        print_error();
    }

    char *program = argv[2];

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        close(pipe_in[1]);
        close(pipe_out[0]);

        if (dup2(pipe_in[0], STDIN_FILENO) == -1 || dup2(pipe_out[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(pipe_in[0]);
        close(pipe_out[1]);

        execlp("/bin/sh", "sh", "-c", program, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipe_in[0]);
        close(pipe_out[1]);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
            if (write(pipe_in[1], buffer, bytes_read) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            bytes_read = read(pipe_out[0], buffer, BUFFER_SIZE);

            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }

        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        close(pipe_in[1]);
        close(pipe_out[0]);

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process exited abnormally\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
