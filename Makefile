
CFLAGS= -std=c++17 -Wall
CC=clang++
LIBS=-lncurses

PROGRAM=main

SRC_FILES= ./main.cpp

OBJ := $(patsubst %.cpp, %.o, $(SRC_FILES))


main:	$(OBJ)
	$(CC) $(OBJ) -o $(PROGRAM) $(CFLAGS) $(LIBS) 


%.o:	%.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(LIBS)
	

.PHONY: main


# Or use -B
force:
	touch main.cpp
	$(MAKE)
