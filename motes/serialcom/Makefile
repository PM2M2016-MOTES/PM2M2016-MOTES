CC=gcc
CFLAGS=-c -Wall -O2
OBJ_DIRECTORY=../obj
BIN_DIRECTORY=../bin

serialcom: serialcom.o
	$(CC) $(OBJ_DIRECTORY)/$^ -o $(BIN_DIRECTORY)/$@

serialcom.o: com.c
	$(CC) $(CFLAGS) $^ -o $(OBJ_DIRECTORY)/$@
