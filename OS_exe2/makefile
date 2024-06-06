# Define compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2

# Define target
TARGET = mync4

# Define source and object files
SRC = mync4.c
OBJ = $(SRC:.c=.o)

# Default target to build the program
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile the object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean the build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Rule to run the program (example usage)
run-server:
	./$(TARGET) -e "./ttt 123456789" -i UDPS4050 -t 10
#./mync3 -e "./ttt 123456789" -i TCPS4050


run-client:
	./$(TARGET) -e "./ttt 123456789" -o UDPC127.0.0.1,4050

# Phony targets to avoid conflicts with files named 'clean', 'run-server', 'run-client'
.PHONY: all clean run-server run-client