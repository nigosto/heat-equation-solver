CC = mpicc
CFLAGS = -Wall -Wextra -Iinclude
EXEC = mpiexec

SRC_DIR = src

SOURCES = $(wildcard $(SRC_DIR)/*.c)
TARGET = main.out
NP ?= 4

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

run: $(TARGET)
	$(EXEC) -np $(NP) $(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)	
