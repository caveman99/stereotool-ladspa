# Makefile for building the Stereo Tool LADSPA plugin

CXX = g++
CC = gcc
CXXFLAGS = -fPIC -Wall -O2
CFLAGS = -fPIC -Wall -O2
LDFLAGS = -shared -L.
LDLIBS = -lStereoTool_intel64

TARGET = stereotool_ladspa.so

SRC_CPP = st_wrapper.cpp
SRC_C = stereotool_ladspa.c

OBJ_CPP = $(SRC_CPP:.cpp=.o)
OBJ_C = $(SRC_C:.c=.o)

all: $(TARGET)

$(OBJ_CPP): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_C): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_CPP) $(OBJ_C)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o $(TARGET)
