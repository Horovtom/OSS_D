client:
	g++ -std=c++11 client.cpp route_cfg_parser.c -o client.o
test: testSocketsServer.cpp testSocketsClient.cpp
	g++ -std=c++11 testSocketsServer.cpp -o testSocketsServer.o -lpthread
	g++ -std=c++11 testSocketsClient.cpp -o testSocketsClient.o -lpthread
clean: 
	rm -f *.o
