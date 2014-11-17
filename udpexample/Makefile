COMPILER=gcc
CFLAGS=-Wall -pedantic -std=gnu99
DFLAGS=-g
SRC=udpexample.c
OUT=udpexample

all:
	$(COMPILER) $(CFLAGS) $(SRC) -o $(OUT)

debug:
	$(COMPILER) $(CFLAGS) $(DFLAGS) $(SRC) -o $(OUT)
clean:
	rm $(OUT)

