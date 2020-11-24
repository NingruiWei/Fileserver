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
    size_t n = count(str.begin(), str.end(), '/');
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

    reverse(result.begin(), result.end());
}

int Fileserver::traverse_pathname(vector<string> &parsed_pathname, fs_inode* curr_inode, fs_direntry curr_entries[]){
    //Need to add hand-over-hand reader locks for traversal. Should be straight forward, but we're lazy right now

    while(parsed_pathname.size() > 1){ //While we still have more than just the new directory/file we're interest in creating
        if(curr_inode->type == 'f'){
            //File should only ever be the last thing along the path, if it's not it should be an error. Double check this assumption
            return -1;
        }

        int found_inode_block = -1;
        for(size_t i = 0; i < FS_MAXFILEBLOCKS; i++){
            if(curr_inode->blocks[i] == 0){ //If the block we're looking at is uninitalized, skip over it
                continue;
            }
            
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; j++){
                if(curr_entries[j].name == parsed_pathname.back()){
                    //Found matching filename, save inode block and break out
                    found_inode_block = curr_entries[j].inode_block;
                    break;
                }
            }

            if(found_inode_block != -1){
                //We already found our inode block, so we can just move onto looking for the next directory.
                break;
            }
        }

        if(found_inode_block == -1){
            //matching filename or directory was not found, the pathname was invalid
            return -1;
        }

        disk_readblock(found_inode_block, curr_inode); //Read in the next inode we're concerned with
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerend with
    }

    return 0;
}

int Fileserver::traverse_single_file(string desired_file, fs_inode* curr_inode, fs_direntry curr_entries[]){
    int found_inode_block = -1;
    for(size_t i = 0; i < FS_MAXFILEBLOCKS; i++){ //Search for the block which has the file we're interested in
        if(curr_inode->blocks[i] == 0){ //If the block we're looking at is uninitalized, skip over it
            continue;
        }
        
        disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
        for(size_t j = 0; j < FS_DIRENTRIES; j++){
            if(curr_entries[j].name == desired_file){
                //Found matching filename, save inode block and break out
                found_inode_block = curr_entries[j].inode_block;
                break;
            }
        }

        if(found_inode_block != -1){
            //We already found our inode block, so we can just move onto looking for the next directory.
            break;
        }
    }

    if(found_inode_block == -1){
        //Unable to find filename/directory name within curr_entries. Error
        return -1;
    }

    disk_readblock(found_inode_block, &curr_inode); //Read in the inode we're going to read from

    return 0;
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
    vector<string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode

    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
    }
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode

    string return_string;

    if(traverse_single_file(parsed_pathname.front(), &curr_inode, curr_entries) == -1){
        //Error, could not find desired file/directory. Error
    }

    for(size_t i = 0; i < FS_MAXFILEBLOCKS; i++){
        if(curr_inode.blocks[i] == 0){ //If the block we're looking at is uninitalized, skip over it
            continue;
        }
        char curr_read[FS_BLOCKSIZE];
        disk_readblock(curr_inode.blocks[i], curr_read); //Read in the current blocks direntries
        return_string += string(curr_read, FS_BLOCKSIZE);
    }

    return return_string;
}

void Fileserver::handle_fs_writeblock(string session, string sequence, string pathname, string block_or_type){
    vector<string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode

    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
    }
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode

    fs_inode prior_inode = curr_inode;
    fs_direntry prior_entries[FS_DIRENTRIES];

    for(size_t i = 0; i < FS_DIRENTRIES; i++){
        prior_entries[i] = curr_entries[i];
    }

    if(traverse_single_file(parsed_pathname.front(), &curr_inode, curr_entries) == -1){
        //Error, could not find desired file/directory. Error
    }
}

void Fileserver::handle_fs_delete(string session, string sequence, string pathname){
    vector<string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);
    
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode

    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
    }
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode

    fs_inode prior_inode = curr_inode;
    fs_direntry prior_entries[FS_DIRENTRIES];

    for(size_t i = 0; i < FS_DIRENTRIES; i++){
        prior_entries[i] = curr_entries[i];
    }

    if(traverse_single_file(parsed_pathname.front(), &curr_inode, curr_entries) == -1){
        //Error, could not find desired file/directory. Error
    }

    //If the inode we're deleting is a directory, it is not allowed to have any subdirectories or files inside of it
    if(curr_inode.type == 'd'){
        for(size_t i = 0; i < FS_MAXFILEBLOCKS; i++){
            if(curr_inode.blocks[i] != 0){
                //Error, there was something within the directory
            }
        }
    }
    else{ //We need to return all of the blocks that file was using to store its data
        for(size_t i = 0; i < FS_MAXFILEBLOCKS; i++){
            if(curr_inode.blocks[i] == 0){
                //Block is uninitalized, skip over it
                continue;
            }
            //Push back the block of data the file was using to store information
        }
    }

    //Delete the inode we were told to and write the change back to the directories above it and its accompanying inode

}

void Fileserver::handle_fs_create(string session, string sequence, string pathname, string type){
    vector<string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    curr_inode.blocks[0] = available_blocks.top();
    available_blocks.pop();
    disk_readblock(0, &curr_inode);

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode

    int success_fail = traverse_pathname(parsed_pathname, &curr_inode, curr_entries);
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode

    if(success_fail == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
    }

    //Create new inode and write the change up to the directories above it (and write that change to its inode)
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
    //available_blocks.resize(FS_MAXFILEBLOCKS);
    for(size_t i = 0; i < FS_MAXFILEBLOCKS; ++i){
        available_blocks.push(i);
    }
    fs_inode root_inode;
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









