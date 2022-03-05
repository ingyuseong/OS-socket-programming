compile:
	gcc -Wall -g3 -fsanitize=address -pthread server.c -o server.o
	gcc -Wall -g3 -fsanitize=address -pthread client.c -o client.o
