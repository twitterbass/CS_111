# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

CC = gcc 
CFLAGS = -Wall -Wextra -g

CFILES = SortedList.h SortedList.c lab2_list.c
PICS = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png


.SILENT:

default: build

build: lab2_list

lab2_list: lab2_list.c SortedList.c
	$(CC) $(CFLAGS) -lpthread -o lab2_list lab2_list.c SortedList.c

profile: lab2_list
	rm -f lab2_list.prof profile.out
	LD_PRELOAD=/usr/local/cs/gperftools-2.7/lib/libprofiler.so CPUPROFILE=lab2_list.prof ./lab2_list --iterations=1000 --threads=12 --sync=s 
	pprof --text lab2_list lab2_list.prof >> profile.out
	pprof --list=list_operation lab2_list lab2_list.prof >> profile.out
	pprof --list=thread_list_insert lab2_list lab2_list.prof >> profile.out
	pprof --list=thread_list_length lab2_list lab2_list.prof >> profile.out
	pprof --list=thread_list_delete lab2_list lab2_list.prof >> profile.out

tests: lab2_list tests.sh
	chmod u+x tests.sh
	./tests.sh

graphs: tests lab2b_list.gp
	chmod u+x lab2b_list.gp
	./lab2b_list.gp
clean:
	rm -f lab2_list lab2_list.prof
	rm -f lab2b-005183495.tar.gz

dist: graphs
	tar -czvf lab2b-005183495.tar.gz Makefile README $(CFILES) $(PICS) lab2b_list.gp lab2b_list.csv tests.sh profile.out 
