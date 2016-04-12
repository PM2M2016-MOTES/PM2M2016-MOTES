CC=gcc
CFLAGS=-c -Wall -O2
OBJ_DIRECTORY=../obj
BIN_DIRECTORY=../bin

sensor: sensor.o
	$(CC) $(OBJ_DIRECTORY)/$^ -o $(BIN_DIRECTORY)/$@

sensor.o: sensor.c
	$(CC) $(CFLAGS) $^ -o $(OBJ_DIRECTORY)/$@
