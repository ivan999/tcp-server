CFLAGS = -Wall -g

server: main.c server.o
	gcc $(CFLAGS) main.c lexer.o session.o server.o -o server

server.o: server.c server.h session.o
	gcc $(CFLAGS) server.c -c

session.o: session.c session.h lexer.o dynarr.h
	gcc $(CFLAGS) session.c -c

lexer.o: lexer.c lexer.h dynarr.h
	gcc $(CFLAGS) lexer.c -c

clean:
	rm server *.o
