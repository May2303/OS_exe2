#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_error() {
    printf("Error\n");
    exit(EXIT_FAILURE);
}

void check_valid_input(char *strategy) {
    if (strlen(strategy) != 9) {
        print_error();
    }
    for (int i = 0; i < 9; i++) {
        if (strategy[i] < '1' || strategy[i] > '9') {
            print_error();
        }
        for (int j = i + 1; j < 9; j++) {
            if (strategy[i] == strategy[j]) {
                print_error();
            }
        }
    }
}

int choose_move(char *board, char *strategy) {
    char msd = strategy[0];
    char lsd = strategy[8];

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (board[j] == strategy[i]) {
                return j;
            }
        }
    }
    return -1; // Should not reach here
}

int check_win(char *board, char player) {
    int win_conditions[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
    for (int i = 0; i < 8; i++) {
        int count = 0;
        for (int j = 0; j < 3; j++) {
            if (board[win_conditions[i][j]] == player) {
                count++;
            }
        }
        if (count == 3) {
            return 1;
        }
    }
    return 0;
}

int check_draw(char *board) {
    for (int i = 0; i < 9; i++) {
        if (board[i] >= '1' && board[i] <= '9') {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_error();
    }

    char *strategy = argv[1];
    check_valid_input(strategy);

    char board[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

    for (int turn = 0; turn < 9; turn++) {
        int move = choose_move(board, strategy);
        printf("%d\n", move + 1); // Adjust for 1-based indexing
        fflush(stdout);

        int player_move;
        scanf("%d", &player_move);
        player_move--;

        if (player_move < 0 || player_move >= 9 || board[player_move] < '1' || board[player_move] > '9') {
            print_error();
        }

        board[player_move] = 'O';

        if (check_win(board, 'O')) {
            printf("I lost\n");
            exit(EXIT_SUCCESS);
        } else if (check_draw(board)) {
            printf("DRAW\n");
            exit(EXIT_SUCCESS);
        }

        move = choose_move(board, strategy);
        printf("%d\n", move + 1); // Adjust for 1-based indexing
        fflush(stdout);

        board[move] = 'X';

        if (check_win(board, 'X')) {
            printf("I win\n");
            exit(EXIT_SUCCESS);
        } else if (check_draw(board)) {
            printf("DRAW\n");
            exit(EXIT_SUCCESS);
        }
    }

    return 0;
}