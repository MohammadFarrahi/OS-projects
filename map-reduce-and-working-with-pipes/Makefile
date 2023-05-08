CC_COMPILER=g++
CC_STANDARD=--std=c++11
CC_FLAGS=-c -g
EXE_NAME=MapReduce.out
CC=${CC_COMPILER} ${CC_STANDARD} ${CC_FLAGS}

all: ${EXE_NAME} ReduceProcess MapProcess

${EXE_NAME}: MainProcess.o utility.o
	g++ --std=c++11 MainProcess.o utility.o -o ${EXE_NAME}

ReduceProcess: ReduceProcess.o utility.o
	g++ --std=c++11 ReduceProcess.o utility.o -o ReduceProcess

MapProcess: MapProcess.o utility.o
	g++ --std=c++11 MapProcess.o utility.o -o MapProcess

MainProcess.o: MainProcess.cpp header.hpp
	${CC} MainProcess.cpp

ReduceProcess.o: ReduceProcess.cpp header.hpp
	${CC} ReduceProcess.cpp

MapProcess.o: MapProcess.cpp header.hpp
	${CC} MapProcess.cpp

utility.o: utility.cpp header.hpp
	${CC} utility.cpp

clean:
	rm *.o
	rm *.out
	rm MapProcess
	rm ReduceProcess