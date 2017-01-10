all: 
	g++ tcp_client_test.cpp -o tcp_client_test.o
	g++ tcp_server_looping_test.cpp -o tcp_server_looping_test.o
	g++ tcp_server_test.cpp -o tcp_server_test.o
	g++ udp_client_test.cpp -o udp_client_test.o
	g++ udp_server_test.cpp -o udp_server_test.o
udp: udp_server_test.cpp udp_client_test.cpp
	g++ -std=c++11 udp_server_test.cpp -o udp_server_test.o
	g++ -std=c++11 udp_client_test.cpp -o udp_client_test.o
clean:
	rm -f *.o
