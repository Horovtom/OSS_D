all:
	g++ -std=c++11 client.cpp route_cfg_parser.c -o client.o
clean: 
	rm -f *.o
