#include "fs_server.h"
#include "fileserver.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
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
            assert(false);
        }

    } // while


} // fill_password_map









