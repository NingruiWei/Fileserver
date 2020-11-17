#include "fs_server.h"
#include <iostream>
#include <unordered_map>
#include "fileserver.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <bits/stdc++.h>

using namespace std;
Fileserver main_fileserver;

int get_port_number(int sockfd) { // adapted from bgreeves-socket-example https://github.com/eecs482/bgreeves-socket-example/blob/master/solution/helpers.h
 	struct sockaddr_in addr;
	socklen_t length = sizeof(addr);
	if (getsockname(sockfd, (struct sockaddr *) &addr, &length) == -1) {
		perror("Error getting port of socket");
		return -1;
	}
	// Use ntohs to convert from network byte order to host byte order.
	return ntohs(addr.sin_port);
 }

 int handle_connection(int connectionfd) {

	printf("New connection %d\n", connectionfd);

	// (1) Receive message from client.

	char msg[512]; // username and password plus null terminator for each and then another plus 1 for the space in between
	memset(msg, 0, sizeof(msg));

	// Call recv() enough times to consume all the data the client sends.
	size_t recvd = 0;
	ssize_t rval;
	int protocol = 0;	
    string username;
	do {
		cout << "doing " << endl;
		// Receive as many additional bytes as we can in one call to recv()
		// (while not exceeding MAX_MESSAGE_SIZE bytes in total).
		rval = recv(connectionfd, msg + recvd, 512 - recvd, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return -1;
		}
		recvd += rval;
		string size; 
        int decrypt;
		if(protocol == 0){
            stringstream ss(msg);
            ss >> username >> size;
            if(!main_fileserver.username_in_map(username)){
                cout << "username inputted was not in the map" << endl;
				close(connectionfd);
                return -1;
            }
            const char* message_size = size.c_str();
            //decrypt = fs_decrypt(password, 
			
			protocol++;
		}

	} while (rval > 0);  // recv() returns 0 when client closes

	// (2) Print out the message
	printf("Client %d says '%s'\n", connectionfd, msg);

	// (4) Close connection
	close(connectionfd);

	return 0;
}



int main(int argc, char** argv){
    // made main_fileserver globally allocated
    main_fileserver.fill_password_map();
    
    int port = 0;
    if (argc == 2) { //port specified
        port = atoi(argv[1]);
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
    }

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    	perror("setsockopt(SO_REUSEADDR) failed");
	}

    struct sockaddr_in addr, cli;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("Failed to bind socket");
    }

    // retrieve port number by reorganizing endian-ness and print
    port = get_port_number(sock);
	cout << "\n@@@ port " << port << endl;
    
    if (listen(sock, 30) == -1) { // spec says queue length of 30 is "sufficient"
        perror("Failed to listen on address");
    }
    socklen_t cli_len = sizeof(cli);

    while (1) {
        int connectionfd = accept(sock, (struct sockaddr *)&cli, &cli_len); // (struct sockaddr *)&cli, &cli_len
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return -1;
		}
		boost::thread t1(boost::ref(handle_connection), connectionfd);
		
		printf("main doing stuff");
    }

    return 0;
}