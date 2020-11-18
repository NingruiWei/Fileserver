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
int sequence_num = 0;

bool check_fs(string original){
	// We must check if sequence number is larger than previous sequence number. The first instance of sequence number is set 
	// during FS_SESSION call
	cout << "SEQUENCE_NUM " << sequence_num << endl;

	string name, session, sequence, block, reappended, data, pathname, type;
	stringstream ss(original);
	ss >> name >> session >> sequence;
	if (stoi(sequence) != sequence_num) {
		cout << "Sequence number does not match" << endl;
		return false;
	}
		if (name == "FS_SESSION") {
			reappended = name + ' ' + session + ' ' + sequence;
		}
		else if (name == "FS_READBLOCK") {
			ss >> block;
			reappended = name + ' ' + session + ' ' + sequence + ' ' + block;
		}
		else if (name == "FS_WRITEBLOCK") {
			ss >> block;
			reappended = name + ' ' + session + ' ' + sequence + ' ' + block;
		}
		else if (name == "FS_CREATE") {
			ss >> pathname >> type;
			reappended = name + ' ' + session + ' ' + sequence + ' ' + pathname + ' ' + type;
		}
		else if (name == "FS_DELETE") {
			ss >> pathname;
			reappended = name + ' ' + session + ' ' + sequence + ' ' + pathname;
		}

	return original == reappended;
}

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
	// Call recv() enough times to consume all the data the client sends.
	// size_t recvd = 0; might need this again to check how many bytes are left to read
	ssize_t rval;
    string username, size, request_message, session, sequence, pathname, block_or_type;
	char msg[1024];
	memset(msg, 0, sizeof(msg));
	do {
		// Receive as many additional bytes as we can in one call to recv()
		// (while not exceeding MAX_MESSAGE_SIZE bytes in total).
		
		rval = recv(connectionfd, msg, 1024, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			close(connectionfd);
			return -1;
		}
		// recvd += rval;
		int decrypt;
		// use stringstream to create a reappended string for validation of request header
		stringstream ss(msg);
		string original = ss.str();
		string re_appended;
        ss >> username >> size;
		re_appended += username + " " + size;
		if (original != re_appended) {
			cout << "Invalid format" << endl;
			close(connectionfd);
			return -1;
		}

		if(!main_fileserver.username_in_map(username)){
			cout << "username inputted was not in the map" << endl;
			close(connectionfd);
			return -1;
		}
		// const char* message_size = size.c_str();
		printf("Client %d says '%s'\n", connectionfd, msg);
		char decrypted_buffer[stoi(size)];
		decrypt = fs_decrypt(main_fileserver.query_map(username).c_str(), &msg[((username+size).size() + 2)], stoi(size), decrypted_buffer);
		if(decrypt == -1){
			cout << "Decryption failed" << endl;
			close(connectionfd);
			return -1;
		}
		
		printf("Client %d says '%s'\n", connectionfd, decrypted_buffer);
		ss.str(decrypted_buffer);
		original = ss.str();
		check_fs(original);
		re_appended = request_message + ' ' + session + ' ' + sequence + ' ' +  pathname + ' ' + block_or_type;
		if (original != re_appended) {
			cout << "Invalid format" << endl;
			close(connectionfd);
			return -1;
		}

		

	} while (rval > 0);  // recv() returns 0 when client closes

	// (2) Print out the message
	// printf("Client %d says '%s'\n", connectionfd, &msg[10]);

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
		
		printf("main doing stuff\n");
		
    }

    return 0;
}