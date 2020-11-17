#include "fs_server.h"
#include "fileserver.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
using namespace std;


Fileserver::Fileserver(){
    session_id = 0;
}
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
// search_map returns true if query is already an username in the map
bool Fileserver::username_in_map(string query){
    return password_map.find(query) != password_map.end();
}

string Fileserver::query_map(string query){
    return password_map[query];
}








