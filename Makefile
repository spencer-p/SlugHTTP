all: main

main: server.o

run: all
	./main
