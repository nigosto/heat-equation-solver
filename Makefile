CC = mpicc
CFLAGS = -Wall -Wextra -Iinclude
EXEC = mpiexec

SRC_DIR = src

SOURCES = $(shell find $(SRC_DIR) -name '*.c')
TARGET = main.out
NP ?= 4

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) -fopenmp -O3 -march=native -o $(TARGET) $(SOURCES)

run: $(TARGET)
	$(EXEC) -np $(NP) $(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)	
