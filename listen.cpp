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
#include <thread>
#include <mutex>
#include <bits/stdc++.h>

using namespace std;

Fileserver main_fileserver;
mutex seq_lock;
extern mutex cout_lock;
int sequence_num = 0;


bool check_fs(string original){
	// We must check if sequence number is larger than previous sequence number. The first instance of sequence number is set 
	// during FS_SESSION call

	string name, session, sequence, block, reappended, data, pathname, type;
	stringstream ss(original);
	ss >> name >> session >> sequence;
	if (stoi(sequence) != sequence_num) {
		cout_lock.lock();
		cout << "Sequence number does not match" << endl;
		cout_lock.unlock();
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
	cout_lock.lock();
	printf("New connection %d\n", connectionfd);
	cout_lock.unlock();

	// Receive message from client.
	// Call recv() enough times to consume all the data the client sends.
	// size_t recvd = 0; might need this again to check how many bytes are left to read
	ssize_t rval;
	char msg[1];
	string clear_text, encrypted;
	bool begin_encrypt = false;
	do {
		memset(msg, 0, sizeof(msg));

		rval = recv(connectionfd, msg, 1, MSG_WAITALL);

		if(!begin_encrypt){
			clear_text += msg;
			if(msg[0] == '\0'){
				begin_encrypt = true;
			}
		}
		else{
			if(msg[0] == '\0'){
				encrypted += '\0';
			}
			encrypted += msg;
		}

		//This is currently a PLACEHOLDER calculation, we need to revisit this to get a more accurate estimate of what the maximum message size can be
		if((clear_text + encrypted).size() > FS_MAXUSERNAME + FS_MAXPATHNAME + FS_BLOCKSIZE + 18){ //message exceeds maximum valid size a message may be, therefore it must be invalid
			cout_lock.lock();
			perror("Message is of an invalid size");
			cout_lock.unlock();
			close(connectionfd);
			return -1;
		}

	} while (rval > 0);  // recv() returns 0 when client closes

	// cout_lock.lock();
	// cout << clear_text << " " << encrypted << endl;
	// cout_lock.unlock();

	stringstream ss(clear_text);
	string username, size;
	ss >> username >> size;

	char decrypted_msg[stoi(size) + 1];
	// cout_lock.lock();
	// cout << main_fileserver.query_map(username) << encrypted << " " << size << " " << endl;
	// cout_lock.unlock();
	int decryption = fs_decrypt(main_fileserver.query_map(username).c_str(), encrypted.c_str(), stoi(size), decrypted_msg);

	if (decryption == -1) {
		cout_lock.lock();
		cout << "Decryption failed" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	if(!check_fs(string(decrypted_msg))){
		cout_lock.lock();
		cout << "Invalid message received" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}
	
	
	string request_message, session, sequence, pathname, block_or_type;
	stringstream ss2(decrypted_msg);
	ss2 >> request_message >> session >> sequence >> pathname >> block_or_type;
	
	cout << request_message << " " << session << " " << sequence << " " << pathname << " " << block_or_type << endl;
	
	string return_message;
	// cout << "request message is " << request_message << endl;
	if(request_message == "FS_SESSION"){
		unsigned int new_session_id = main_fileserver.handle_fs_session(session, sequence);
		return_message = to_string(new_session_id) + ' '  + sequence + '\0';
		char return_encrypt[(return_message.size()*2) + 64];
		int encryption = fs_encrypt(main_fileserver.query_map(username).c_str(), return_message.c_str(), return_message.size(), return_encrypt);
		if(encryption == -1){
			cout_lock.lock();
			cout << "Encryption failed" << endl;
			cout_lock.unlock();
			close(connectionfd);
			return -1;
		}

		string appender;
		for (size_t i = 0; i < 33; i++) {
			appender += return_encrypt[i];
			if (return_encrypt[i] == '\0') {
				cout << "we got em" << endl;
			}
		}
		cout << appender << endl;


		return_message = string(return_encrypt);
		return_message = to_string(return_message.size() + 2) + "\0" + return_message + "\0" + "]";
		cout << return_message << endl;
	}
	else if(request_message == "FS_READBLOCK"){
		main_fileserver.handle_fs_readblock(session, sequence, pathname, block_or_type);

	}
	else if(request_message == "FS_WRITEBLOCK"){
		main_fileserver.handle_fs_writeblock(session, sequence, pathname, block_or_type);
	}
	else if(request_message == "FS_CREATE"){
		main_fileserver.handle_fs_create(session, sequence, pathname);
	}
	else if(request_message == "FS_DELETE"){
		main_fileserver.handle_fs_delete(session, sequence, pathname);
	}
	else{
		cout_lock.lock();
		cout << "Client request command not recognized" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	//Close connection and return successfully
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
		cout_lock.lock();
        perror("Failed to create socket");
		cout_lock.unlock();
    }

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		cout_lock.lock();
    	perror("setsockopt(SO_REUSEADDR) failed");
		cout_lock.unlock();
	}

    struct sockaddr_in addr, cli;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		cout_lock.lock();
        perror("Failed to bind socket");
		cout_lock.unlock();
    }

    // retrieve port number by reorganizing endian-ness and print
    port = get_port_number(sock);
	cout_lock.lock();
	cout << "\n@@@ port " << port << endl;
	cout_lock.unlock();
    
    if (listen(sock, 30) == -1) { // spec says queue length of 30 is "sufficient"
        perror("Failed to listen on address");
    }
    socklen_t cli_len = sizeof(cli);

    while (1) {
        int connectionfd = accept(sock, (struct sockaddr *)&cli, &cli_len); // (struct sockaddr *)&cli, &cli_len
		if (connectionfd == -1) {
			cout_lock.lock();
			perror("Error accepting connection");
			cout_lock.unlock();
			return -1;
		}

		thread t1(handle_connection, connectionfd);
		t1.detach();
		cout_lock.lock();
		printf("main doing stuff\n");
		cout_lock.unlock();
    }

    return 0;
}