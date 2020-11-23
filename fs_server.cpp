#include "fs_server.h"
#include "fileserver.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
#include <mutex>
#include <string.h>
#include <stdio.h>
#include <algorithm>
using namespace std;


Fileserver::Fileserver(){}
Fileserver::~Fileserver(){}
void split_string_spaces(vector<string> &result, string str){
    size_t n = count(str.begin(), str.end(), "/");
    result.reserve(n);
    string temp;
    for (char c: str){
        if(result.size() == 0){
            result.push_back("/");
            continue;
        }
        if (c == '/'){
            result.push_back(temp);
            temp.clear();
        }
        else{
            temp += c;
        }
    }

    
}
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

int Fileserver::handle_fs_session(string session, string sequence, string username){
    session_map_entry temp;
    temp.sequence_num = stoi(sequence); // do we have to dynamically allocate and just store as a pointer? HMMM
    temp.username = username;
    session_map.push_back(temp);
    return session_map.size() - 1;
}

string Fileserver::handle_fs_readblock(string session, string sequence, string pathname, string block_or_type){
    return "data";
}

void Fileserver::handle_fs_writeblock(string session, string sequence, string pathname, string block_or_type){}

void Fileserver::handle_fs_delete(string session, string sequence, string pathname){}

void Fileserver::handle_fs_create(string session, string sequence, string pathname, string type){
    vector<string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode* curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, curr_inode);

    fs_direntry curr_entires[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode

    /*
        I think this all could be a function, because we'll need the exact same function for readblock, writeblock, delete, and create
    */
    while(parsed_pathname.size() > 1){ //While we still have more than just the new directory/file we're interest in creating
        int found_inode_block = -1;
        for(int i = 0; i < FS_MAXFILEBLOCKS; i++){
            if(curr_inode->blocks[i] == 0){ //If the block we're looking at is uninitalized, skip over it
                continue;
            }
            
            disk_readblock(curr_inode->blocks[i], curr_entires); //Read in the current blocks direntries
            for(int j = 0; j < FS_DIRENTRIES; j++){
                if(curr_entires[j].inode_block == 0){
                    continue; //Uninitalized block, skip over it
                }

                if(curr_entires[j].name == parsed_pathname.back()){
                    //Found matching filename, save inode block and break out
                    found_inode_block = curr_entires[j].inode_block;
                    break;
                }
            }

            if(found_inode_block != -1){
                //We already found our inode block, so we can just move onto looking for the next directory.
                break;
            }
        }

        disk_readblock(found_inode_block, curr_inode); //Read in the next inode we're concerned with
        parsed_pathname.erase(parsed_pathname.begin()); //Remove the first element of the vector so that we look for the next directory we're concerend with
    }

    //We now shold have the 
}

// search_map returns true if query is already an username in the map
bool Fileserver::username_in_map(string query){
    return password_map.find(query) != password_map.end();
}

string Fileserver::query_map(string query){
    return password_map[query];
}

int Fileserver::query_session_map(int session){
    return session_map[session].sequence_num;
}

int Fileserver::valid_session_range(){
    return session_map.size() - 1;
}

void Fileserver::init_fs() {
    fs_inode* root_inode;
    disk_readblock(0, root_inode);
    assert(root_inode != nullptr);

    if (root_inode->size > 0) {
        for (size_t i = 0; i < root_inode->size; i++) {
            // push to free_data_blocks vector/array for fileserver
            cout << root_inode->blocks[i] << endl;
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









