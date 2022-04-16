CXX=g++
CFLAGS=-std=c++17 -O2 -Wall -g
CXXFLAGS=-std=c++17 -O2 -Wall -g

TARGET:=server.o
SOURCE:=$(wildcard ../*.cpp)
OBJS=./buffer.cpp ./HTTPrequest.cpp ./HTTPresponse.cpp ./HTTPconnection.cpp \
     ./timer.cpp ./epoller.cpp ./webserver.cpp ./main.cpp

$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS)  $(OBJS) -o ./bin/$(TARGET) -pthread

