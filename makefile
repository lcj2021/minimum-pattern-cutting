mpc2: graph.o Utils.o main.o
	g++ graph.o Utils.o main.o -o mpc2
graph.o: graph.cpp graph.h
	g++ -c graph.cpp -o graph.o
Utils.o: Utils.cpp Utils.h
	g++ -c Utils.cpp -o Utils.o
main.o: main.cpp
	g++ -c main.cpp -o main.o
	