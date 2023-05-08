CC=g++
FLAGS=--std=c++11 -w -pthread
EXE_NAME=ImageFilters.out

all: ImageFilters.out

ImageFilters.out: main.cpp
	${CC} ${FLAGS} main.cpp -o ${EXE_NAME}

clean:
	rm *.out