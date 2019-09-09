# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

CC = gcc
CFLAGS = -Wall -Wextra -g 

CFILES = SortedList.h SortedList.c lab2_add.c lab2_list.c
LOGS = lab2_add.csv lab2_list.csv
SCRIPTS = test_add.sh test_list.sh
GPLOTS = lab2_add.gp lab2_list.gp
PICS1 = lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png
PICS2 = lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
.SILENT:

default: build

build: lab2_add lab2_list

lab2_add: lab2_add.c
	$(CC) $(CFLAGS) -lpthread -o lab2_add lab2_add.c

lab2_list: lab2_list.c SortedList.c
	$(CC) $(CFLAGS) -lpthread -o lab2_list lab2_list.c SortedList.c

tests: build test_add.sh test_list.sh
	chmod u+x test_add.sh
	chmod u+x test_list.sh
	./test_add.sh
	./test_list.sh

graphs: lab2_add.gp lab2_list.gp tests
	chmod u+x lab2_add.gp
	chmod u+x lab2_list.gp
	./lab2_add.gp
	./lab2_list.gp

dist: Makefile README graphs
	tar -czvf lab2a-005183495.tar.gz Makefile README $(CFILES) $(GPLOTS) $(SCRIPTS) $(LOGS) $(PICS1) $(PICS2)

clean:
	rm -f lab2_add lab2_list 
	rm -f lab2a-005183495.tar.gz
