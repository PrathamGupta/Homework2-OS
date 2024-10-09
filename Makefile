# Makefile for compiling the flow program

CC = gcc
CFLAGS = -Wall -g
TARGET = flow

all: $(TARGET)

$(TARGET): flow.o
	$(CC) $(CFLAGS) -o $(TARGET) flow.o

flow.o: flow.c
	$(CC) $(CFLAGS) -c flow.c

clean:
	rm -f *.o $(TARGET)
