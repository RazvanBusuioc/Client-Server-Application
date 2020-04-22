CFLAGS = -Wall -g
PORT = 1409
IP_SERVER = 127.0.0.1
CLIENT_ID = user1

build: server.cpp client.cpp database.cpp
	g++ -c database.cpp
	g++ -c server.cpp
	g++ -c client.cpp
	g++ database.o server.o -o server
	g++ client.cpp -o subscriber


run_server:
	./server ${PORT}
run_subscriber:
	./subscriber  ${CLIENT_ID} ${IP_SERVER} ${PORT}

clean:
	rm server subscriber *o

