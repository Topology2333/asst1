V=8

all: myexp

logger.o: logger.cpp logger.h CS149intrin.h CS149intrin.cpp
	g++ -D VECTOR_WIDTH=$(V) -c logger.cpp

CS149intrin.o: CS149intrin.cpp CS149intrin.h logger.cpp logger.h
	g++ -D VECTOR_WIDTH=$(V) -c CS149intrin.cpp

myexp: CS149intrin.o logger.o main.cpp
	g++ -D VECTOR_WIDTH=$(V) -I../common logger.o CS149intrin.o main.cpp -o myexp

clean:
	rm -rf *.o myexp *~ compile_commands.json .cache/ .gdb_history
