all: main

main: server.o

run: all
	./main

clean:
	rm -rf server.o main
