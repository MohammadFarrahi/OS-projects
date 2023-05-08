CC_COMPILER=gcc

all: server client

server: server.c constants.h
	${CC_COMPILER} -w server.c -o server

client: client.c constants.h
	${CC_COMPILER} -w client.c -o client

clean:
	rm server
	rm client