# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 5

.SILENT:

default: lab1a.c
	gcc -Wall -Wextra -g -o lab1a lab1a.c

clean:
	rm -f *.tar.gz lab1a
dist: lab1a.c Makefile
	tar -czvf lab1a-005183495.tar.gz README Makefile lab1a.c 
