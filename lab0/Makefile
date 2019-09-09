# NAME: John Tran
# EMAIL: johntran627@gmail.com
# ID: 005183495
# SLIPDAYS: 3


.SILENT:

default: lab0.c
	gcc -Wall -Wextra -g -o lab0 lab0.c

clean:
	rm -f lab0-005183495.tar.gz lab0

dist: Makefile README backtrace.png breakpoint.png lab0.c 
	tar -czvf lab0-005183495.tar.gz Makefile README lab0.c breakpoint.png backtrace.png

check: default allTests
	if [[ -s Err.log ]]; then \
		cat Err.log; \
		rm -f Err.log; \
	else \
		echo "All smoke tests passed."; \
	fi;


# Smoke test cases:
normalTest:
	echo "Lord of the Rings" > input.txt;
	./lab0 --input input.txt --output output.txt; \
	if [[ $$? -ne 0 ]]; then \
		echo "Smoke test failed: normalTest" >> Err.log; \
	fi;
	rm -f *.txt;

badArg:
	./lab0 --meow &> /dev/null; \
	if [[ $$? -ne 1 ]]; then \
		echo "Smoke test failed: badArg" >> Err.log; \
	fi; 

badInput:
	echo "Hello, kitten!!!" > input.txt;
	chmod u-r input.txt;
	./lab0 --input input.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
		echo "Smoke test failed: badInput" >> Err.log; \
	fi; 
	rm -f *.txt;

badOutput:
	echo "Meow meow!!!" > output.txt;
	chmod u-w output.txt;
	./lab0 --output output.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
		echo "Smoke test failed: badOutput" >> Err.log; \
	fi;
	rm -f *.txt;

segfaultTest:
	./lab0 --catch --segfault &> /dev/null; \
	if [[ $$? -ne 4 ]]; then \
		echo "Smoke test failed: segfaultTest" >> Err.log; \
	fi;


allTests: normalTest badArg badInput badOutput segfaultTest 
