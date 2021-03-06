CFLAGS += -std=gnu11\
		   -Wall -Werror -pedantic\
		   -O3

LIB := slughttp.o
APP := main

all: $(APP)

$(APP): $(LIB)

run: all
	./$(APP)

clean:
	rm -rf $(APP) $(LIB)
