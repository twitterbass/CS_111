# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 5

.SILENT:

default: client server

client: lab1b-client.c
	gcc -Wall -Wextra -g -o lab1b-client lab1b-client.c -lmcrypt

server: lab1b-server.c
	gcc -Wall -Wextra -g -o lab1b-server lab1b-server.c -lmcrypt

clean:
	rm -f lab1b-client lab1b-server lab1b-005183495.tar.gz

dist: README Makefile lab1b-client.c lab1b-server.c my.key
	tar -czvf lab1b-005183495.tar.gz lab1b-client.c lab1b-server.c README Makefile my.key
