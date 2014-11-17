
FLAGS := -Wall -pedantic -std=gnu99
COMPILER := gcc $(FLAGS)
COMPILE := $(COMPILER) -c
LINK := $(COMPILER)
LINKFLAGS := -lm

all: 
	make client
	make server

client: client.o
	$(LINK) $@.o -o tcp$@

server: server.o
	$(LINK) $@.o -o tcp$@ $(LINKFLAGS)

%.o: %.c
	$(COMPILE) $*.c -o $*.o
	
clean:
	@if [ `find|grep ".o" |wc -l` -gt 0 ] ; then rm *.o ; fi
	@if [ -x tcpclient ] ; then rm tcpclient ; fi
	@if [ -x tcpserver ] ; then rm tcpserver ; fi
	

	

