# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

.SILENT:

lab4b: lab4b.c
	gcc -Wall -Wextra -g -o lab4b lab4b.c -lm -lmraa

check: lab4b 
	chmod u+x test.sh
	./test.sh

clean:
	rm -f lab4b lab4b-005183495.tar.gz

dist: lab4b.c README Makefile test.sh
	tar -czvf lab4b-005183495.tar.gz lab4b.c README Makefile test.sh
