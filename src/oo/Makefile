CC = g++

MYFLAGS = -Wall -g -O2 --std=c++17

BINDIR = ./

CXREF = cxref

LIBS = -lprotobuf -lpthread

OBJFILES = ../ZoneManager.o ../GameManager.o ../proto/dawnmud.pb.o main.o

CXREF_FILES = ZoneManager.cpp GameManager.cpp

default: all

all:
	$(MAKE) dawn

dawn : $(OBJFILES)
	$(CC) -o dawn $(OBJFILES) $(LIBS)


clean:
	rm -f *.o
