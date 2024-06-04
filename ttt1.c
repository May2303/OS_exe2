#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to validate the strategy input
int validate_strategy(const char *strategy) {
    if (strlen(strategy) != 9) { 
        return 0;
    }

    int digits[10] = {0};
    for (int i = 0; i < 9; i++) {
        if (!isdigit(strategy[i]) || strategy[i] == '0') { 
            return 0;
        }
        int digit = strategy[i] - '0';
        if (digits[digit] > 0) {
            return 0;
        }
        digits[digit]++;
    }

    return 1;
}

void print_board(char *board) {
    printf("Board:\n");
    printf(" %c | %c | %c \n", board[0], board[1], board[2]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[3], board[4], board[5]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[6], board[7], board[8]);
}
int check_winner(char *board, char mark) {
    int win_conditions[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // columns
        {0, 4, 8}, {2, 4, 6}             // diagonals
    };

    for (int i = 0; i < 8; i++) {
        if (board[win_conditions[i][0]] == mark &&
            board[win_conditions[i][1]] == mark &&
            board[win_conditions[i][2]] == mark) {
            return 1;
        }
    }

    return 0;
}

int is_full(char *board) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == ' ') {
            return 0;
        }
    }
    return 1;
}

void ttt(const char *strategy) {
    if (!validate_strategy(strategy)) {
        printf("Error\n");
        return;
    }

    int move_order[9];
    for (int i = 0; i < 9; i++) {
        move_order[i] = strategy[i] - '0';
    }

    char board[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    char ai_mark = 'X';
    char human_mark = 'O';

    print_board(board);  // Initial empty board

    while (1) {
        // AI move
        for (int i = 0; i < 9; i++) {
            int move = move_order[i] - 1;
            if (board[move] == ' ') {
                board[move] = ai_mark;
                printf("%d\n", move_order[i]);
                fflush(stdout);
                break;
            }
        }

        print_board(board);  // Print board after AI move

        if (check_winner(board, ai_mark)) {
            printf("I win\n");
            return;
        }

        if (is_full(board)) {
            printf("DRAW\n");
            return;
        }

        // Human move
        int human_move;
                if (scanf("%d", &human_move) != 1 || human_move < 1 || human_move > 9 || board[human_move - 1] != ' ') {
            printf("Error\n");
            return;
        }
        board[human_move - 1] = human_mark;

        print_board(board);  // Print board after human move

        if (check_winner(board, human_mark)) {
            printf("I lost\n");
            return;
        }

        if (is_full(board)) {
            printf("DRAW\n");
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error\n");
        return 1;
    }

    ttt(argv[1]);
    return 0;
}

