# Multiple Access Chatting Program using Multithread
* Term project / KECE340 Operating System ([Prof. Lynn Choi](http://it.korea.ac.kr/?_ga=2.35055464.854245486.1646472198-316617093.1632121575))

## Description
* Topic: Socket Programming
  * Build a simple chatting program using multithreading

* Server
  * Should be able to relay chat messages between clients
  * Should be able to handle multiple clients at the same time (Implement multithreading)
  * Messages must be sent to the connected clients

* Client
  * Should be able to connect to the server when entering the server IP and PORT
  * User input: chat message / Output: received message from the server

## Environment
* ami-0f8b8babb98cc66d0 (64-bit(x86))
* Ubuntu Server 20.04 LTS (HVM)
* gcc (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0
* GNU Make 4.2.1

## Compile
```bash
# Makefile
compile:
	gcc -Wall -g3 -fsanitize=address -pthread server.c -o server.o
	gcc -Wall -g3 -fsanitize=address -pthread client.c -o client.o
```
