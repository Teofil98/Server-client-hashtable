CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++20 -g -lpthread

all: server client

server: src/server.cpp src/include/
	$(CXX) $(CXXFLAGS) src/server.cpp -o server 

client: src/client.cpp src/include/
	$(CXX) $(CXXFLAGS) src/client.cpp -o client 

clean:
	rm server client 
