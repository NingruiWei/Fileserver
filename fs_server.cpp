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

fs_inode Fileserver::get_curr_inode() {
    return curr_inode;
}

void Fileserver::init_fs() {
    disk_readblock(0, &root_inode);
    if (root_inode.size > 0) {
        for (size_t i = 0; i < root_inode.size; i++) {
            // push to free_data_blocks vector/array for fileserver
            cout << root_inode.blocks[i] << endl;
        }
    }
}

// - reads into entries array the directory at dir_inode[i]
// - can and probably will be called within a loop through dir_inode's blocks
// - entries is the table of dir_entries associated with the corresponding block value at i for dir_inode
// - whenever we create a file or directory, we need to update size and blocks in the root inode
void Fileserver::read_directory(fs_direntry *entries, fs_inode *dir_inode, size_t i) {
    disk_readblock(dir_inode->blocks[i], &entries);
}








