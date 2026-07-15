CXX ?= g++
CXXFLAGS ?= -O3 -Wall -pthread

all: sender receiver

# This compiles the new sender.cpp into ./sender
sender: sender.cpp
	$(CXX) $(CXXFLAGS) -o sender sender.cpp

# This compiles the new receiver.cpp into ./receiver
receiver: receiver.cpp
	$(CXX) $(CXXFLAGS) -o receiver receiver.cpp

clean:
	rm -f sender receiver
