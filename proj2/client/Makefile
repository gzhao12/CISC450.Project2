CC= /usr/bin/gcc
all:	 udpclient udpclienttest

udpclient: udpclient.c;
	${CC} udpclient.c -o udpclient

udpclienttest: udpclient.c;
	${CC} -DDEBUG udpclient.c -o udpclienttest

clean:
	rm udpclient udpclienttest
