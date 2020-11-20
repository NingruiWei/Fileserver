#include "fs_server.h"
#include "fileserver.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
#include <mutex>
using namespace std;


Fileserver::Fileserver(){}
Fileserver::~Fileserver(){}

void Fileserver::fill_password_map(){
    string username, password;
    while(cin >> username >> password){
        if(password_map.find(username)== password_map.end()){
            password_map[username] = password;
        }
        else{
            cout << "insertion to map assertion error";
            assert(false);
        }

    } // while


} // fill_password_map

int Fileserver::handle_fs_session(string session, string sequence){
    session_map.push_back(stoi(sequence));
    return session_map.size() - 1;
}

string Fileserver::handle_fs_readblock(string session, string sequence, string pathname, string block_or_type){
    return "data";
}

void Fileserver::handle_fs_writeblock(string session, string sequence, string pathname, string block_or_type){}

void Fileserver::handle_fs_delete(string session, string sequence, string pathname){}

void Fileserver::handle_fs_create(string session, string sequence, string pathname){}

// search_map returns true if query is already an username in the map
bool Fileserver::username_in_map(string query){
    return password_map.find(query) != password_map.end();
}

string Fileserver::query_map(string query){
    return password_map[query];
}

int Fileserver::query_session_map(int session){
    return session_map[session];
}

int Fileserver::valid_session_range(){
    return session_map.size() - 1;
}








