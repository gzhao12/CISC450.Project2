CC= /usr/bin/gcc
all:	 udpserver udpservertest

udpserver: udpserver.c;
	${CC} udpserver.c -o udpserver -lm

udpservertest: udpserver.c;
	${CC} -DDEBUG udpserver.c -o udpservertest -lm

clean:
	rm udpserver udpservertest
