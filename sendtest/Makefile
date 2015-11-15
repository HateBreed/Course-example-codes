COMPILE=gcc -Wall -std=gnu99 -g

all:
	$(COMPILE) sendtests.c -o sendtests
	$(COMPILE) sendtestc.c -o sendtestc

allOnceNoWait:
	$(COMPILE) sendtests.c -o sendtests -D__ALL_ONCE_DONTWAIT
	$(COMPILE) sendtestc.c -o sendtestc -D__ALL_ONCE_DONTWAIT

allOnce:
	$(COMPILE) sendtests.c -o sendtests -D__ALL_ONCE
	$(COMPILE) sendtestc.c -o sendtestc -D__ALL_ONCE

bufferSizeTenth:
	$(COMPILE) sendtests.c -o sendtests -D__BUFTENTH
	$(COMPILE) sendtestc.c -o sendtestc -D__BUFTENTH
	
clean:
	@rm sendtests
	@rm sendtestc
