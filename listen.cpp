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
uint sequence_num = 0;


bool check_fs(string original){
	// We must check if sequence number is larger than previous sequence number. The first instance of sequence number is set 
	// during FS_SESSION call

	string name, block, reappended, data, pathname, type;
	uint session, sequence;
	stringstream ss(original);
	ss >> name >> session >> sequence;

	// cout_lock.lock();
	// cout << "decrypted message " << name << " " << session << " " << sequence << endl;
	// cout_lock.unlock();

	if(name != "FS_SESSION" && main_fileserver.valid_session_range(session)){
		cout_lock.lock();
		cout << "Session number is invalid" << endl;
		cout_lock.unlock();
		return false;
	}

	if (name != "FS_SESSION" && main_fileserver.query_session_map_sequence(session, sequence)) {
		cout_lock.lock();
		cout << "Sequence number is invalid" << endl;
		cout_lock.unlock();
		return false;
	}

	if (name == "FS_SESSION") {
		reappended = name + ' ' + to_string(session) + ' ' + to_string(sequence);
	}
	else if (name == "FS_READBLOCK") {
		ss >> pathname >> block;
		reappended = name + ' ' + to_string(session) + ' ' + to_string(sequence) + ' ' + pathname + ' ' + block;
	}
	else if (name == "FS_WRITEBLOCK") {
		ss >> pathname >> block;
		reappended = name + ' ' + to_string(session) + ' ' + to_string(sequence) + ' ' + pathname + ' ' + block;
	}
	else if (name == "FS_CREATE") {
		ss >> pathname >> type;
		reappended = name + ' ' + to_string(session) + ' ' + to_string(sequence) + ' ' + pathname + ' ' + type;
	}
	else if (name == "FS_DELETE") {
		ss >> pathname;
		reappended = name + ' ' + to_string(session) + ' ' + to_string(sequence) + ' ' + pathname;
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

void encrypt_return_message(string return_message, int* error_check, const string username, bool readblock, int connectionfd, string data_string){
	char return_encrypt[((return_message.size() + data_string.size() + 1)*2) + 64];

	int message_size = return_message.size() + 1;
	if (readblock) {
		message_size += FS_BLOCKSIZE;
	}
	char message_to_encrypt[message_size];
	memcpy(message_to_encrypt, return_message.c_str(), return_message.size() + 1);
	if (readblock) {
		cout_lock.lock();
		cout << "DATA: " << data_string << endl;
		cout_lock.unlock();
		memcpy(message_to_encrypt + return_message.size() + 1, data_string.c_str(), FS_BLOCKSIZE);
	}

	int encryption = fs_encrypt(main_fileserver.query_map(username).c_str(), message_to_encrypt, message_size, return_encrypt);

	if(encryption == -1){
		cout_lock.lock();
		cout << "Encryption failed" << endl;
		cout_lock.unlock();
		*error_check = -1;
		//close(connectionfd);
		return;
	}

	send(connectionfd, to_string(encryption).c_str(), to_string(encryption).size() + 1, MSG_NOSIGNAL);
	send(connectionfd, return_encrypt, encryption, MSG_NOSIGNAL);
}

int decrypt_message(char *decrypted_msg, string &encrypted, string &username, int size_encrypted, int connectionfd) {
	// original encrypted string does not contain <NULL> in the right location, so
	// this is to recreate what the c_string should be when passed into fs_decrypt()
	char encrypted_cstr[size_encrypted];
	for (size_t i = 0; i < encrypted.size(); i++) {
		encrypted_cstr[i] = encrypted[i];
	}
	encrypted_cstr[size_encrypted - 1] = ']';
	//cout << encrypted_cstr[size_encrypted - 1] << endl;

	int decryption = fs_decrypt(main_fileserver.query_map(username).c_str(), encrypted_cstr, size_encrypted, decrypted_msg);
	// cout << "DECRYPTION: " << decryption << endl;

	if (decryption == -1) {
		cout_lock.lock();
		cout << "Decryption failed" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	// MUST FINISH IMPLEMENTING

	if(!check_fs(string(decrypted_msg))){
		cout_lock.lock();
		cout << "Invalid message received" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}
	return decryption;
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
	memset(msg, 0, sizeof(msg));
	string clear_text;//, encrypted;
	//string size_encrypted;
	do {
		rval = recv(connectionfd, msg, 1, MSG_WAITALL);
		if (rval == -1) {
			close(connectionfd);
			return -1;
		}

		//Just recv until first null character, then we'll check if username is valid and if it is, we can recv size for entire encrypted message all at once.

		if(msg[0] == '\0'){
			//We should now have username (space) size of encrypted message, we can break out
			clear_text += string(msg, 1);
			break;
		}
		clear_text += string(msg, 1);

		//This is currently a PLACEHOLDER calculation, it's a conservative estimate for how long the cleartext message coule be (which is the max length of a username, a space, the maximum length of the encrypted message, and a single null character)
		if(clear_text.size() > FS_MAXUSERNAME + 6){ //message exceeds maximum valid size a message may be, therefore it must be invalid
			cout_lock.lock();
			perror("Message is of an invalid size");
			cout_lock.unlock();
			close(connectionfd);
			return -1;
		}

	} while (rval > 0);  // recv() returns 0 when client closes

	//Check username validity, then recv size for encrypted message (which we can unencrypt the same way we currently do)
	stringstream cleartext_for_encrypted(clear_text);
	string username, size;
	if (!(cleartext_for_encrypted >> username >> size)) {
		close(connectionfd);
		return -1;
	}
	if(!main_fileserver.username_in_map(username) || username.size() > FS_MAXUSERNAME){
		cout_lock.lock();
		cout << "Invalid username" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	char encrypted_msg[stoi(size)];
	if (recv(connectionfd, encrypted_msg, stoi(size), MSG_WAITALL) != stoi(size)) {
		close(connectionfd);
		return -1;
	} //recv exactly the number of bits the sender says the encrypted message
	string encrypted = string(encrypted_msg, stoi(size));
	
	//decrypt the message and store in char[]
	char decrypted_msg[stoi(size)];
	int decrypted_len = decrypt_message(decrypted_msg, encrypted, username, stoi(size), connectionfd);
	
	if (decrypted_len == -1) {
		cout_lock.lock();
		cout << "DECRYPTION HAD INVALID STUFF" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	string request_message, pathname, block_or_type;
	uint session, sequence;
	stringstream ss2(decrypted_msg);
	ss2 >> request_message >> session >> sequence >> pathname >> block_or_type;
	
	string return_message;
	cout_lock.lock();
	cout << string(decrypted_msg) << endl;
	cout << "request message is " << request_message << endl;
	cout_lock.unlock();
	
	if(request_message != "FS_SESSION" && (main_fileserver.query_session_map_username(session) != username)){
		close(connectionfd);
		return -1;
	}

	if(request_message == "FS_SESSION"){
		unsigned int new_session_id = main_fileserver.handle_fs_session(session, sequence, username);
		return_message = to_string(new_session_id) + " " + to_string(sequence);
		int fail_check = 0;
		encrypt_return_message(return_message, &fail_check, username, 0, connectionfd, "");

		if(fail_check == -1){
			close(connectionfd);
			return -1;
		}

	}
	else if(request_message == "FS_READBLOCK"){
		cout_lock.lock();
        cout << "INSIDE FS_READBLOCK LINE 225" << endl;
		cout_lock.unlock();
		char read_data[FS_BLOCKSIZE];
		if(main_fileserver.handle_fs_readblock(session, sequence, pathname, block_or_type, read_data) == -1){
			close(connectionfd);
			return -1;
		}
		string data_string = string(read_data, 512);
		return_message = to_string(session) + ' ' + to_string(sequence);
		int fail_check = 0;
		encrypt_return_message(return_message, &fail_check, username, true, connectionfd, data_string);

		if(fail_check == -1){
			close(connectionfd);
			return -1;
		}

		//send(connectionfd, appender.c_str(), appender.size(), 0);

	}
	else if(request_message == "FS_WRITEBLOCK"){
		size_t specified_size = decrypted_len;
		auto header_len = strnlen(decrypted_msg, specified_size); // must check if header size is valid later on
		auto data_len = specified_size - header_len - 1;
		cout_lock.lock();
		cout << "DATA_LEN = " << data_len << endl;
		cout << "HEADER_LEN = " << header_len << endl;
		cout << "specified_size = " << specified_size << endl;
		cout_lock.unlock();
		char data[FS_BLOCKSIZE];
		memcpy(data, decrypted_msg + header_len + 1, data_len); // eventually if replace FS_BLOCKSIZE with the size of data actually passed in for error checking
		cout_lock.lock();
		cout << data << endl;
		cout_lock.unlock();

		if(main_fileserver.handle_fs_writeblock(session, sequence, pathname, block_or_type, data) == -1){
			close(connectionfd);
			return -1;
		}
		return_message = to_string(session) + ' ' + to_string(sequence);

		int fail_check = 0;
		encrypt_return_message(return_message, &fail_check, username, 0, connectionfd, "");

		if(fail_check == -1){
			close(connectionfd);
			return -1;
		}

		//send(connectionfd, appender.c_str(), appender.size(), 0);
	}
	else if(request_message == "FS_CREATE"){
		
		if(main_fileserver.handle_fs_create(session, sequence, pathname, block_or_type) == -1){
			close(connectionfd);
			return -1;
		};

		return_message = to_string(session) + ' ' + to_string(sequence);

		int fail_check = 0;
		encrypt_return_message(return_message, &fail_check, username, 0, connectionfd, "");

		if(fail_check == -1){
			close(connectionfd);
			return -1;
		}

		//send(connectionfd, appender.c_str(), appender.size(), 0);
	}
	else if(request_message == "FS_DELETE"){
		if(main_fileserver.handle_fs_delete(session, sequence, pathname) == -1){
			close(connectionfd);
			return -1;
		}

		return_message = to_string(session) + ' ' + to_string(sequence);

		int fail_check = 0;
		encrypt_return_message(return_message, &fail_check, username, 0, connectionfd, "");

		if(fail_check == -1){
			close(connectionfd);
			return -1;
		}

		//send(connectionfd, appender.c_str(), appender.size(), 0);
	}
	else{
		cout_lock.lock();
		cout << "Client request command not recognized" << endl;
		cout_lock.unlock();
		close(connectionfd);
		return -1;
	}

	//Close connection and return successfully
	main_fileserver.insert_sequence(sequence, session);
	close(connectionfd);
	return 0;
}

int main(int argc, char** argv){
    // made main_fileserver globally allocated
    main_fileserver.fill_password_map();
	main_fileserver.init_fs();
    
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
		cout_lock.lock();
		cout << "START WHILE" << endl;
		cout_lock.unlock();
        int connectionfd = accept(sock, (struct sockaddr *)&cli, &cli_len); // (struct sockaddr *)&cli, &cli_len
		if (connectionfd == -1) {
			cout_lock.lock();
			perror("Error accepting connection");
			cout_lock.unlock();
			return -1;
		}

		thread t1(handle_connection, connectionfd);
		//t1.join();// Only for single threading testing
		t1.detach();
		cout_lock.lock();
		printf("main doing stuff\n");
		cout_lock.unlock();
    }

    return 0;
}