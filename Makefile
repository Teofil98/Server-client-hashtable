CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -g 

all: server client

server: src/server.cpp
	$(CXX) $(CXXFLAGS) src/server.cpp -o server 

client: src/client.cpp
	$(CXX) $(CXXFLAGS) src/client.cpp -o client 

clean:
	rm server client 
