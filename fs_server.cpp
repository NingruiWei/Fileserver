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
void deepcopy(fs_direntry* new_one, fs_direntry* original){
    for(size_t i = 0; i < FS_DIRENTRIES; ++i){
        strcpy(new_one[i].name, original[i].name);
        new_one[i].inode_block = original[i].inode_block;
    }
}
void Fileserver::lock_on_disk(std::string path, bool shared_lock){
    lock_guard<mutex> fs_lock(fileserver_lock);
    directory_lock_map[path].lock_uses++;
    if(shared_lock){
        //If this is a shared lock (meant for reading) we need to do a shared locking function
        directory_lock_map[path].lock.lock_shared();
    }
    else{
        //Not a shared lock (meant for writing)
         directory_lock_map[path].lock.lock();
    }
}

void Fileserver::unlock_on_disk(std::string path, bool shared_lock){
    lock_guard<mutex> fs_lock(fileserver_lock);
    on_disk_lock *to_unlock = &directory_lock_map[path];
    to_unlock->lock_uses--;
    
    if(shared_lock){
        to_unlock->lock.unlock_shared();
    }
    else{
        to_unlock->lock.unlock();
    }

    if(to_unlock->lock_uses == 0){
        directory_lock_map.erase(path);
    }
}

bool direntry_full(fs_inode* curr){ // passed in should the inode in question
    for(size_t i = 0; i < curr->size; ++i){

    }
    return true; // no room found so it is full
}

bool Fileserver::blocks_full(){
    return available_blocks.empty();
}

// adds block to inode passed in and returns the block number. If returns -1, then we dont have space either in the inode or we ran out of blocks.
int Fileserver::add_block_to_inode(fs_inode* curr){
    lock_guard<mutex> fs_lock(fileserver_lock);
    if(curr->size == FS_MAXFILEBLOCKS || blocks_full()) { // reached max space in inode already or ran out of blocks in memory
        return -1;
    }

    curr->blocks[curr->size] = available_blocks.top();
    curr->size++;
    available_blocks.pop();

    // curr_entries[0].inode_block = curr->blocks[curr->size - 1];
    // strcpy(curr_entries[0].name, filename.c_str());
    // for(size_t i = 1; i < FS_DIRENTRIES; i++){
    //     curr_entries[i].inode_block = 0;
    // }

    return curr->blocks[curr->size - 1];
}

fs_inode create_inode(std::string type, std::string owner){
    fs_inode temp;
    temp.type = type[0];
    strcpy(temp.owner, owner.c_str());
    temp.size = 0;
    return temp;
}

int insert_into_curr_entries(fs_direntry* curr, fs_direntry temp){
    for (size_t i = 0; i < FS_DIRENTRIES; ++i){ // loop through the array
        if(curr[i].inode_block == 0 ){ // if the current element is empty
            curr[i].inode_block = temp.inode_block;
            strcpy(curr[i].name, temp.name);
            return 0;
        }
    }

    return -1;
}

void split_string_spaces(vector<std::string> &result, std::string str){
    size_t n = count(str.begin(), str.end(), '/');
    result.reserve(n);
    std::string temp;
    bool first = true;
    for (char c: str){
        if (c == '/'){
            if(first){
                first = false;
                continue;
            }
            result.push_back(temp);
            temp.clear();
        }
        else{
            temp += c;
        }
    }
    result.push_back(temp);

    reverse(result.begin(), result.end());
}

bool direntries_full(fs_direntry entries[]){
    for(size_t i = 0; i < FS_DIRENTRIES; i++){
        if(entries[i].inode_block == 0){
            return false;
        }
    }
    return true;
}
// afterwards parent_inode block will be the block of the directory that we're going to insert stuff into, parent_entries block will be the correct entries block, or 0 if one does not exist
// curr_inode should be the root coming in.
int Fileserver::traverse_pathname_delete(vector<std::string> &parsed_pathname, fs_inode* curr_inode, fs_direntry curr_entries[],
 int &curr_inode_block, int &parent_entries_block, fs_inode* parent_inode, int& parent_entries_index, int& parent_inode_block, string username){
   //Need to add hand-over-hand reader locks for traversal. Should be straight forward, but we're lazy right now
    curr_inode_block = 0;
    parent_entries_block = 0;
    parent_inode_block = 0;
    string travelled_path = "/";
    bool shared_status = (parsed_pathname.size() - 1) != 0;
    path_lock parent_lock(travelled_path, shared_status, this);
    int loop = 0;
    while(parsed_pathname.size() > 1){  
        for(size_t i = 0; i < curr_inode->size; ++i){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; ++j){
                if(strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
                    parent_inode_block = curr_inode->blocks[i];
                    goto loop;
                }
            }
        }
        //matching filename or directory was not found, the pathname was invalid
        return -1;
        
        loop:
        travelled_path += parsed_pathname.back();
        if(parsed_pathname.size() > 1){
            travelled_path += "/";
        }
        shared_status = (parsed_pathname.size() >= 2);
        path_lock child_lock(travelled_path, shared_status, this);
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0 ){
            return -1;
        }
        parent_lock.swap_lock(&child_lock);
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }

    parent_inode->type = curr_inode->type;
    strcpy(parent_inode->owner, curr_inode->owner);
    parent_inode->size = curr_inode->size;
    for(size_t k = 0; k < curr_inode->size; ++k){
        parent_inode->blocks[k] = curr_inode->blocks[k];
    }

    // now always need to go down one more layer
    for(size_t i = 0; i < curr_inode->size; i++){
        disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
        for(size_t j = 0; j < FS_DIRENTRIES; j++){
            if(strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
                curr_inode_block = curr_entries[j].inode_block;
                parent_entries_block = curr_inode->blocks[i];
                parent_entries_index = j;
                goto end;
            }
        }
    }
    
   
    end:
    return 0;
}
int Fileserver::traverse_pathname_create(vector<std::string> &parsed_pathname, fs_inode* curr_inode,
             fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block, string username){
    parent_inode_block = 0;
    parent_entries_block = 0;
    fs_direntry fake_entries[FS_DIRENTRIES];
    int loop = 0;
    string travelled_path = "/";
    bool shared_status = (parsed_pathname.size() - 1) != 0;
    path_lock parent_lock(travelled_path, shared_status, this);

    while(parsed_pathname.size() > 1){ //While we still have more than just the new directory/file we're interest in creating
        
        if(curr_inode->type == 'f'){
            //File should only ever be the last thing along the path, if it's not it should be an error. Double check this assumption
            return -1;
        }

        
        for(size_t i = 0; i < curr_inode->size; i++){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; j++){
                if(strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
                    parent_inode_block = curr_entries[j].inode_block;
                    goto loop;
                }
            }
        }
        //matching filename or directory was not found, the pathname was invalid
        return -1;
        
        loop:
        travelled_path += parsed_pathname.back();
        if(parsed_pathname.size() > 1){
            travelled_path += "/";
        }
        shared_status = (parsed_pathname.size() >= 2);
        path_lock child_lock(travelled_path, shared_status, this);
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0){
                return -1; // check to see that the username matches directory
            }
        parent_lock.swap_lock(&child_lock);
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }
    
    // once we have found the inode we are inserting in we see if there's already space or not in the thing
    bool found = false;
    for(size_t i = 0; i < curr_inode->size; i++){
        disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
        
        for(size_t j = 0; j < FS_DIRENTRIES; j++){
            if(strcmp(parsed_pathname.back().c_str(), curr_entries[j].name) == 0){
                return -1;
            }
            if(!found && curr_entries[j].inode_block == 0){
                
                found = true;
                deepcopy(fake_entries, curr_entries);
                //Found matching filename, save inode block and break out
                parent_entries_block = curr_inode->blocks[i];
                
            }
        }
    }
    deepcopy(curr_entries, fake_entries);
    //If the inode has no initalized blocks, then we need to make it a new block
    if(curr_inode->size == 0 || direntries_full(curr_entries)){
        parent_entries_block = 0;
    }
    
    return 0;
}
int Fileserver::traverse_pathname(vector<std::string> &parsed_pathname, fs_inode* curr_inode, 
            fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block, bool fs_read, string username){
    
    parent_inode_block = 0;
    parent_entries_block = 0;

    string travelled_path = "/";
    bool shared_status = (parsed_pathname.size() != 0 || fs_read);
    path_lock parent_lock(travelled_path, shared_status, this);
    int loop = 0;
    while((parsed_pathname.size() > 0)){ //While we still have more than just the new directory/file we're interest in creating
        
        if(curr_inode->type == 'f'){
            //File should only ever be the last thing along the path, if it's not it should be an error. Double check this assumption
            return -1;
        }
        for(size_t i = 0; i < curr_inode->size; i++){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; j++){
                if(strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
                    parent_inode_block = curr_entries[j].inode_block;
                    goto loop;
                }
            }
        }
        //matching filename or directory was not found, the pathname was invalid
        return -1;
        
        loop:
        travelled_path += parsed_pathname.back();
        if(parsed_pathname.size() > 1){
            travelled_path += "/";
        }
        shared_status = (parsed_pathname.size() != 0 || fs_read);
        path_lock child_lock(travelled_path, shared_status, this);
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0){
                return -1; // check to see that the username matches directory
            }
        parent_lock.swap_lock(&child_lock);
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }
    
    return 0;
}



void Fileserver::fill_password_map(){
    std::string username, password;
    while(cin >> username >> password){
        if(password_map.find(username) == password_map.end()){
            password_map[username] = password;
        }
        else{
            cout << "insertion to map assertion error";
            assert(false);
        }

    } // while


} // fill_password_map

int Fileserver::handle_fs_session(std::string session, std::string sequence, std::string username){
    session_map_entry temp;
    temp.sequence_num = stoi(sequence); 
    temp.username = username;
    session_map.push_back(temp);
    return session_map.size() - 1;
}

int Fileserver::handle_fs_readblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type, char* return_arr){
    vector<std::string> parsed_pathname; 
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int parent_inode, parent_entries;
    string username = session_map[stoi(session)].username;

    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries, parent_inode, parent_entries, true, username) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
        return -1;
    }
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode
    size_t block = stoi(block_or_type);
    if(block >= curr_inode.size){ return -1;}
    int read_block = curr_inode.blocks[block];
    disk_readblock(read_block, return_arr);
    return 0;
}

int Fileserver::handle_fs_writeblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type, char* data){
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s
    string remain = parsed_pathname.front();
    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int parent_inode_block = 0, parent_entries_block;
    string username = session_map[stoi(session)].username;
    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries, parent_inode_block, parent_entries_block, false, username) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
        return -1;
    }
   
    

    // we always need to add a new block to blocks
    size_t block = stoi(block_or_type);
    int write_to_block;
    if(block > curr_inode.size){return -1;}
    else if(block == curr_inode.size){
        write_to_block = add_block_to_inode(&curr_inode);
        disk_writeblock(write_to_block, data);
        disk_writeblock(parent_inode_block, &curr_inode);
        }
    else{
        write_to_block = curr_inode.blocks[block];
        disk_writeblock(write_to_block, data);
    }
    return 0;
   
}
bool no_entries(fs_direntry* curr){
    for(size_t i = 0; i < FS_DIRENTRIES; ++i){
        if(curr[i].inode_block != 0){
            return false;
        }
    }
    return true;
}

int Fileserver::handle_fs_delete(std::string session, std::string sequence, std::string pathname){
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode, parent_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);
    
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int curr_inode_block = 0, parent_entries = 0, parent_entries_index = -1, parent_node_block;
    string username = session_map[stoi(session)].username;
    if(traverse_pathname_delete(parsed_pathname, &curr_inode, curr_entries, curr_inode_block, parent_entries, &parent_inode, parent_entries_index, parent_node_block, username) == -1){
        return -1;
    }
    disk_readblock(curr_inode_block, &curr_inode);
    if(strcmp(curr_inode.owner, username.c_str()) != 0){
        return -1;
    }

    // ok so we parent_entries_index is the index in parent_entries
    // parent_entries is the block number of the direntry table
    // we want
    // curr_inode is the inode in block "curr_inode_block"
    if(curr_inode.type == 'f'){
        for(size_t i = 0; i < curr_inode.size; ++i){
            available_blocks.push(curr_inode.blocks[i]);
        }
    }
    else if(curr_inode.size > 0){
        return -1;
    }
    available_blocks.push(curr_inode_block);
    // after this we have freed up the blocks at the very end eg. paul/klee, the klee block would be freed up now

    curr_entries[parent_entries_index].inode_block = 0; // this should clear it
    if(no_entries(curr_entries)){ // if curr_entries is empty we then free up the whole block it's in since it's useless now
        available_blocks.push(parent_entries);
        //disk_readblock(parent_inode_block, )
        for(size_t i = 0; i < parent_inode.size; ++i){
            if((int) parent_inode.blocks[i] == parent_entries){
                for(size_t j = i; j < parent_inode.size - 1; ++j){
                    parent_inode.blocks[j] = parent_inode.blocks[j + 1];
                }
                --parent_inode.size;
                disk_writeblock(parent_node_block, &parent_inode);
                goto end;
            }
        }
        
    }
    else{
        disk_writeblock(parent_entries, curr_entries);
    }
   end:
   return 0;

}

int Fileserver::handle_fs_create(std::string session, std::string sequence, std::string pathname, std::string type){
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    split_string_spaces(parsed_pathname, pathname); //Parse pathname on /'s

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    
    disk_readblock(0, &curr_inode);
    string username = session_map[stoi(session)].username;
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    for(size_t i = 0; i < FS_DIRENTRIES; ++i){ // initializes it all to 0 for now
        curr_entries[i].inode_block = 0;
    }
    int parent_inode_block = 0, parent_entries_block = 0;
    
    if(traverse_pathname_create(parsed_pathname, &curr_inode, curr_entries, parent_inode_block, parent_entries_block, username) == -1){
        return -1;
    }
    // if no room currently to add new direntry

    bool curr_entries_full = false;
    if(parent_entries_block == 0){
        parent_entries_block = add_block_to_inode(&curr_inode);
        if (parent_entries_block == -1){
            //If there were no free blocks left, then you cannot add a new block to the inode, ERROR
        }
        for(size_t i = 0; i < FS_DIRENTRIES; ++i){ // initializes it all to 0 for now
            curr_entries[i].inode_block = 0;
            }
        curr_entries_full = true;
    }
    

    //Create new inode and direntry for the file/directory we're creating
    fs_inode temp = create_inode(type, username);
    fs_direntry temp_entry;
    fileserver_lock.lock();
    temp_entry.inode_block = available_blocks.top();
    available_blocks.pop();
    fileserver_lock.unlock();
    const char * remaining_path = parsed_pathname.back().c_str();
    strcpy(temp_entry.name, remaining_path);

    for(size_t i = 0; i < FS_DIRENTRIES; i++){
        if(curr_entries[i].inode_block == 0){
            curr_entries[i] = temp_entry;
            break;
        }
    }

    disk_writeblock(temp_entry.inode_block, &temp); //Write new inode into its block
    disk_writeblock(parent_entries_block, &curr_entries); //Change curr_direntires to have the updated information
    if(curr_entries_full){
        disk_writeblock(parent_inode_block, &curr_inode); //Change curr_inode to acknowledge change to curr_inode
    }
    return 0;
}

// search_map returns true if query is already an username in the map
bool Fileserver::username_in_map(std::string query){
    return password_map.find(query) != password_map.end();
}

string Fileserver::query_map(std::string query){
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
    for(size_t i = 1; i < FS_MAXFILEBLOCKS; ++i){
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









