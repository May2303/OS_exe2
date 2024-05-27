#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int print_error() {
    printf("Error\n");
    return -1;
}

void check_valid_input(int *strategy) {
    int count[10] = {0}; // Array to count occurrences of each digit
    for (int i = 0; i < 9; i++) {
        if (strategy[i] < 1 || strategy[i] > 9 || count[strategy[i]] > 0) {
            print_error();
        }
        count[strategy[i]]++;
    }
}

int choose_move(int *board, int *strategy) {
    int msd = strategy[0];
    int lsd = strategy[8];
    int used[9] = {0};  // Array to track used positions

    // Mark positions already used on the board
    for (int i = 0; i < 9; i++) {
        if (board[i] == 11 || board[i] == 22) {
            used[i] = 1;
        }
    }

    // Find the first available move in the strategy
    for (int i = 0; i < 9; i++) {
        if (strategy[i] >= msd && strategy[i] <= lsd && !used[strategy[i] - 1]) {
            used[strategy[i] - 1] = 1;
            return strategy[i] - 1;
        }
    }

    // If all moves are used or strategy is invalid, return -1
    return -1;
}

int check_win(int *board, int player) {
    int win_options[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
    for (int i = 0; i < 8; i++) {
        int count = 0;
        for (int j = 0; j < 3; j++) {
            if (board[win_options[i][j]] == player) {
                count++;
            }
        }
        if (count == 3) {
            return 1;
        }
    }
    return 0;
}

int check_draw(int *board) {
    for (int i = 0; i < 9; i++) {
        if (board[i] >= 1 && board[i] <= 9) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_error();
    }

    char *strategy_str = argv[1];
    int strategy[9];

    // Convert strategy string to integer array
    for (int i = 0; i < 9; i++) {
        strategy[i] = strategy_str[i] - '0';
    }

    check_valid_input(strategy);

    int board[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (int turn = 0; turn < 9; turn++) {
        int AImove = choose_move(board, strategy);
        printf("%d\n", AImove + 1); // Adjust for 1-based indexing
        //fflush(stdout);
        board[AImove] = 22; // 'X' represented as 22

        if (check_win(board, 22)) { // 'X' represented as 22
            printf("I win\n");
            exit(EXIT_SUCCESS);
        } else if (check_draw(board)) {
            printf("DRAW\n");
            exit(EXIT_SUCCESS);
        }

        int player_move;
        scanf("%d", &player_move);
        player_move--;

        if (player_move < 0 || player_move >= 9 || board[player_move] < 1 || board[player_move] > 9) {
            print_error();
        }

        board[player_move] = 11; // 'O' represented as 11

        if (check_win(board, 11)) { // 'O' represented as 11
            printf("I lost\n");
            exit(EXIT_SUCCESS);

        } else if (check_draw(board)) {
            printf("DRAW\n");
            exit(EXIT_SUCCESS);
        }
/*
        AImove = choose_move(board, strategy);
        printf("%d\n", AImove + 1); // Adjust for 1-based indexing
       // fflush(stdout);

*/
        
    }
    return 0;
}
