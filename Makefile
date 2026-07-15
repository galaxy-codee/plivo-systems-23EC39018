CXX ?= g++
CXXFLAGS ?= -O3 -Wall -pthread

all: sender receiver

sender: sender.cpp
	$(CXX) $(CXXFLAGS) -o sender sender.cpp

receiver: receiver.cpp
	$(CXX) $(CXXFLAGS) -o receiver receiver.cpp

clean:
	rm -f sender receiver
