# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 00518349599

.SILENT: 

default: lab4c_tcp lab4c_tls

lab4c_tcp: lab4c_tcp.c
	gcc -lmraa -lm -Wall -Wextra -o lab4c_tcp lab4c_tcp.c

lab4c_tls: lab4c_tls.c
	gcc -lmraa -lm -lssl -lcrypto -Wall -Wextra -o lab4c_tls lab4c_tls.c

clean:
	rm -f lab4c_tcp lab4c_tls lab4c-005183495.tar.gz

dist: lab4c_tcp.c lab4c_tls.c Makefile README
	tar -czvf lab4c-005183495.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README

