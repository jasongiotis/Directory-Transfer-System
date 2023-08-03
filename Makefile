
compile:
	g++ dataServer.cpp -o dataServer -pthread -std=c++17 -lstdc++fs
	g++ clientpc/remoteClient.cpp -o clientpc/remoteClient -pthread -std=c++17 -lstdc++fs

clean:
	rm dataServer -rf
	rm clientpc/remoteClient -rf