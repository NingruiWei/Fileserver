#include "fs_server.h"
#include "fileserver.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <string.h>
#include <stdio.h>
//These are commented out so that we avoid submitting our sleeps during an actual submission. Uncomment for concurrency testing
//#include <thread>
//#include <chrono>
#include <algorithm>
using namespace std;

extern mutex cout_lock;
unordered_map<std::string, on_disk_lock> directory_lock_map;
std::mutex available_blocks_mutex;
std::mutex directory_lock_map_mutex;
std::mutex fileserver_shared_lock;

Fileserver::Fileserver(){}

Fileserver::~Fileserver(){}

void deepcopy(fs_direntry* new_one, fs_direntry* original){
    for(size_t i = 0; i < FS_DIRENTRIES; ++i){
        strcpy(new_one[i].name, original[i].name);
        new_one[i].inode_block = original[i].inode_block;
    }
}

void lock_on_disk(std::string path, bool shared_lock){
    directory_lock_map_mutex.lock();
    on_disk_lock *to_lock = &directory_lock_map[path];
    directory_lock_map[path].lock_uses++;
    directory_lock_map_mutex.unlock();
    if(shared_lock){
        //If this is a shared lock (meant for reading) we need to do a shared locking function
        to_lock->lock.lock_shared();
        cout_lock.lock();
        cout << path << " path shared locked with " << directory_lock_map[path].lock_uses << " uses" << endl;
        cout_lock.unlock();
    }
    else{
        //Not a shared lock (meant for writing)
        to_lock->lock.lock();
        cout_lock.lock();
        cout << path << " path private locked with " << directory_lock_map[path].lock_uses << " uses" << endl;
        cout_lock.unlock();
    }
}

void unlock_on_disk(std::string path, bool shared_lock){
    lock_guard<mutex> directory_map(directory_lock_map_mutex);

    if(directory_lock_map.find(path) == directory_lock_map.end()){
        return; //path does not exist in the map
    }

    on_disk_lock *to_unlock = &directory_lock_map[path];
    to_unlock->lock_uses--;
    
    if(shared_lock){
        to_unlock->lock.unlock_shared();
        cout_lock.lock();
        cout << path << " path shared unlocked with " << directory_lock_map[path].lock_uses << " uses" << endl;
        cout_lock.unlock();
    }
    else{
        to_unlock->lock.unlock();
        cout_lock.lock();
        cout << path << " path private unlocked with " << directory_lock_map[path].lock_uses << " uses" << endl;
        cout_lock.unlock();
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
    lock_guard<mutex> fs_lock(available_blocks_mutex);
    if(curr->size == FS_MAXFILEBLOCKS || blocks_full()) { // reached max space in inode already or ran out of blocks in memory
        return -1;
    }

    curr->blocks[curr->size] = *(available_blocks.begin());
    curr->size++;
    available_blocks.erase(available_blocks.begin());

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

int split_string_spaces(vector<std::string> &result, std::string str){
    if (str[str.size() - 1] == '/'){
        return -1;
    }
    if (str[0] != '/'){
        return -1;
    }
    std::string temp;
    bool first = true;
    for (char c: str){
        if(c == ' '  || c == '\0'){
            return -1;
        }
        if (c == '/'){
            if(first){
                first = false;
                continue;
            }
            if(temp.size() == 0){
                return -1;
            }
            result.push_back(temp);
            temp.clear();
        }
        else{
            temp += c;
        }
    }
    result.push_back(temp);

    std::reverse(result.begin(), result.end());
    return 0;
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
                                            int &curr_inode_block, int &parent_entries_block, fs_inode* parent_inode, int& parent_entries_index, int& parent_inode_block, string username,
                                            path_lock &return_parent_lock, path_lock &return_to_delete_lock){
   //Need to add hand-over-hand reader locks for traversal. Should be straight forward, but we're lazy right now
    curr_inode_block = 0;
    parent_entries_block = 0;
    parent_inode_block = 0;
    string travelled_path = "/";
    bool shared_status = parsed_pathname.size() > 1;
    int loop = 0;
    while(parsed_pathname.size() > 1){  
        for(size_t i = 0; i < curr_inode->size; ++i){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; ++j){
                if(curr_entries[j].inode_block != 0 && strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
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
        shared_status = (parsed_pathname.size() > 2); //The file you're deleting and its parent directory both should be private locked
        path_lock child_lock(travelled_path, shared_status);
        return_parent_lock.swap_lock(&child_lock);
        child_lock.manual_unlock();
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0 ){
            return -1;
        }
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }

    if(parsed_pathname.back().size() > FS_MAXFILENAME){
        return -1;
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
            if(curr_entries[j].inode_block != 0 && strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
                curr_inode_block = curr_entries[j].inode_block;
                parent_entries_block = curr_inode->blocks[i];
                parent_entries_index = j;
                goto end;
            }
        }
    }
    return -1; // didn't find a matching filename to delete;
   
    end:
    travelled_path += parsed_pathname.back();
    return_to_delete_lock.new_lock(travelled_path, false);
    // cout_lock.lock();
    // cout << "Parent lock: " << return_parent_lock.pathname << endl;
    // cout << "Delete lock: " << return_to_delete_lock.pathname << endl;
    // cout_lock.unlock();

    return 0;
}

int Fileserver::traverse_pathname_create(vector<std::string> &parsed_pathname, fs_inode* curr_inode,
             fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block, string username, path_lock &return_parent_lock){
    parent_inode_block = 0;
    parent_entries_block = 0;
    fs_direntry fake_entries[FS_DIRENTRIES];
    int loop = 0;
    string travelled_path = "/";
    bool shared_status = parsed_pathname.size() > 1;

    while(parsed_pathname.size() > 1){ //While we still have more than just the new directory/file we're interest in creating
        
        if(curr_inode->type == 'f'){
            //File should only ever be the last thing along the path, if it's not it should be an error. Double check this assumption
            return -1;
        }

        
        for(size_t i = 0; i < curr_inode->size; i++){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; j++){
                if( curr_entries[j].inode_block != 0 && strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
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
        shared_status = (parsed_pathname.size() > 2); //You only need to private lock the parent directory of where you're creating your file
        path_lock child_lock(travelled_path, shared_status);
        return_parent_lock.swap_lock(&child_lock);
        child_lock.manual_unlock();
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0){
                return -1; // check to see that the username matches directory
        }
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }
    
    if(curr_inode->type == 'f'){
        //You should only be able to create files or directories within directories, not within files
        return -1;
    }

    // once we have found the inode we are inserting in we see if there's already space or not in the thing
    bool found = false;
    for(size_t i = 0; i < curr_inode->size; i++){
        disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
        
        for(size_t j = 0; j < FS_DIRENTRIES; j++){
            if(curr_entries[j].inode_block!= 0 && strcmp(parsed_pathname.back().c_str(), curr_entries[j].name) == 0){
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
    
    // cout_lock.lock();
    // cout << "Parent lock: " << return_parent_lock.pathname << endl;
    // cout_lock.unlock();

    return 0;
}
int Fileserver::traverse_pathname(vector<std::string> &parsed_pathname, fs_inode* curr_inode, 
            fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block, bool fs_read, string username, path_lock &return_parent_lock){
    
    parent_inode_block = 0;
    parent_entries_block = 0;

    string travelled_path = "/";
    bool shared_status = true; //The root is always shared for read and write (you cannot write the root and read is always shared)
    int loop = 0;
    while((parsed_pathname.size() > 0)){ //While we still have more than just the new directory/file we're interest in creating
        
        if(curr_inode->type == 'f'){
            //File should only ever be the last thing along the path, if it's not it should be an error. Double check this assumption
            return -1;
        }
        for(size_t i = 0; i < curr_inode->size; i++){
            disk_readblock(curr_inode->blocks[i], curr_entries); //Read in the current blocks direntries
            for(size_t j = 0; j < FS_DIRENTRIES; j++){
                if((curr_entries[j].inode_block != 0) && strcmp(curr_entries[j].name, parsed_pathname.back().c_str()) == 0){
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
        shared_status = (parsed_pathname.size() > 1 || fs_read); //We only privately lock the thing we're interested in writing to, reading is always a shared lock
        path_lock child_lock(travelled_path, shared_status);
        return_parent_lock.swap_lock(&child_lock);
        child_lock.manual_unlock();
        disk_readblock(parent_inode_block, curr_inode); //Read in the next inode we're concerned with
        if(loop == 0 && strcmp(curr_inode->owner, username.c_str()) != 0){
                return -1; // check to see that the username matches directory
            }
        parsed_pathname.pop_back(); //Remove the first element of the vector so that we look for the next directory we're concerned with
        ++loop;
    }
    
    // cout_lock.lock();
    // cout << "Parent lock: " << return_parent_lock.pathname << endl;
    // cout << "Child lock: " << return_child_lock.pathname << endl;
    // cout_lock.unlock();

    return 0;
}



void Fileserver::fill_password_map(){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    std::string username, password;
    while(cin >> username >> password){
        if(username.size() > FS_MAXUSERNAME || password.size() > FS_MAXPASSWORD){
            continue;
        }
        if(password_map.find(username) == password_map.end()){
            password_map[username] = password;
        }
        else{
            cout_lock.lock();
            cout << "insertion to map assertion error";
            cout_lock.unlock();
            assert(false);
        }

    } // while


} // fill_password_map

int Fileserver::handle_fs_session(uint session, uint sequence, std::string username){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    session_map_entry temp;
    temp.sequence_num = sequence; 
    temp.username = username;
    session_map.push_back(temp);
    return session_map.size() - 1;
}

int Fileserver::handle_fs_readblock(uint session, uint sequence, std::string pathname, std::string block_or_type, char* return_arr){
    if (pathname == "/"){
        return -1;
    }
    if(pathname.size() > FS_MAXPATHNAME){
        return -1;
    }
    int block = stoi(block_or_type);
    if (block > 123 || block < 0){
        return -1;
    }
    vector<std::string> parsed_pathname; 
    if (split_string_spaces(parsed_pathname, pathname) == -1){
        return -1;
    } 

    path_lock parent_lock("/", true);

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int parent_inode, parent_entries;
    fileserver_shared_lock.lock();
    string username = session_map[session].username;
    fileserver_shared_lock.unlock();

    //Uncomment to test concurrency for users being blocked (or not blocked) on shared pathname
    // if(stoi(sequence) < 4){
    //     cout_lock.lock();
    //     cout << "t1 blocking" << endl;
    //     cout_lock.unlock();
    //     std::this_thread::sleep_for(std::chrono::seconds(90));
    //     cout_lock.lock();
    //     cout << "user1 finished blocking" << endl;
    //     cout_lock.unlock();
    // }
    // else{
    //     cout_lock.lock();
    //     cout << "past t1 blocking" << endl;
    //     cout_lock.unlock();
    // }


    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries, parent_inode, parent_entries, true, username, parent_lock) == -1){
        //travesal failed due to an invalid pathname, tell listen to close connection
        return -1;
    }
    //Upon success, pathname should return parsed_pathname with a single entry which is the file/directory we're interested in
    //Curr_inode should point to the directory inode which holds the inode we're interested in and curr_entires is the direntries associated with that inode
    
    if((size_t) block >= curr_inode.size){ return -1;}
    int read_block = curr_inode.blocks[block];
    disk_readblock(read_block, return_arr);
    return 0;
}

int Fileserver::handle_fs_writeblock(uint session, uint sequence, std::string pathname, std::string block_or_type, char* data){
     if (pathname == "/"){
        return -1;
    }
    if(pathname.size() > FS_MAXPATHNAME){
        return -1;
    }
    int block = stoi(block_or_type);
    if (block > 123 || block < 0){
        return -1;
    }
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    if (split_string_spaces(parsed_pathname, pathname) == -1){
        return -1;
    } 
    string remain = parsed_pathname.front();

    path_lock parent_lock("/", true);

    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int parent_inode_block = 0, parent_entries_block;
    fileserver_shared_lock.lock();
    string username = session_map[session].username;
    fileserver_shared_lock.unlock();

    //Uncomment to test concurrency for users being blocked (or not blocked) on shared pathname

    if(traverse_pathname(parsed_pathname, &curr_inode, curr_entries, parent_inode_block, parent_entries_block, false, username, parent_lock) == -1){
        //traversal failed due to an invalid pathname, tell listen to close connection
        return -1;
    }

    // if(stoi(sequence) < 3){
    //     cout_lock.lock();
    //     cout << "user1 blocking" << endl;
    //     cout_lock.unlock();
    //     std::this_thread::sleep_for(std::chrono::seconds(90));
    //     cout_lock.lock();
    //     cout << "user1 finished blocking" << endl;
    //     cout_lock.unlock();
    // }
    // else{
    //     cout_lock.lock();
    //     cout << "user2 past user1 blocking" << endl;
    //     cout_lock.unlock();
    // }
   
    if(curr_inode.type != 'f'){
        return -1;
    }
    

    // we always need to add a new block to blocks
    
    int write_to_block;
    if( (size_t) block > curr_inode.size){return -1;}
    else if((size_t)block == curr_inode.size){
        int check = add_block_to_inode(&curr_inode);
        if(check == -1){
            return -1;
        }
        write_to_block = check;
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

int Fileserver::handle_fs_delete(uint session, uint sequence, std::string pathname){
    if (pathname == "/"){
        return -1;
    }
    if(pathname.size() > FS_MAXPATHNAME){
        return -1;
    }
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    if (split_string_spaces(parsed_pathname, pathname) == -1){
        return -1;
    }

    path_lock parent_lock("/", parsed_pathname.size() > 1), to_delete_lock;

    fs_inode curr_inode, parent_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at
    disk_readblock(0, &curr_inode);
    
    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    int curr_inode_block = 0, parent_entries = 0, parent_entries_index = -1, parent_node_block;
    fileserver_shared_lock.lock();
    string username = session_map[session].username;
    fileserver_shared_lock.unlock();

    //Uncomment to test concurrency for users being blocked (or not blocked) on shared pathname
    // if(username == "user1"){
    //     cout_lock.lock();
    //     cout << "user1 blocking" << endl;
    //     cout_lock.unlock();
    //     std::this_thread::sleep_for(std::chrono::seconds(90));
    //     cout_lock.lock();
    //     cout << "user1 finished blocking" << endl;
    //     cout_lock.unlock();
    // }
    // else{
    //     cout_lock.lock();
    //     cout << "user2 past user1 blocking" << endl;
    //     cout_lock.unlock();
    // }

    if(traverse_pathname_delete(parsed_pathname, &curr_inode, curr_entries, curr_inode_block, parent_entries, &parent_inode, parent_entries_index, parent_node_block, username, parent_lock, to_delete_lock) == -1){
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
    lock_guard<mutex> fs_lock(available_blocks_mutex);
    if(curr_inode.type == 'f'){
        for(size_t i = 0; i < curr_inode.size; ++i){
            available_blocks.insert(curr_inode.blocks[i]);
        }
    }
    else if(curr_inode.size > 0){
        return -1;
    }
    available_blocks.insert(curr_inode_block);
    // after this we have freed up the blocks at the very end eg. paul/klee, the klee block would be freed up now

    curr_entries[parent_entries_index].inode_block = 0; // this should clear it
    if(no_entries(curr_entries)){ // if curr_entries is empty we then free up the whole block it's in since it's useless now
        available_blocks.insert(parent_entries);
        //disk_readblock(parent_inode_block, )
        for(size_t i = 0; i < parent_inode.size; ++i){
            if((int) parent_inode.blocks[i] == parent_entries){
                for(size_t j = i; j < parent_inode.size - 1; ++j){
                    parent_inode.blocks[j] = parent_inode.blocks[j + 1];
                }// inside for
                --parent_inode.size;
                disk_writeblock(parent_node_block, &parent_inode);
                    goto end;
            } // if 
        }
    }
    else{
        disk_writeblock(parent_entries, curr_entries);
    }

   end:
   return 0;

}

int Fileserver::handle_fs_create(uint session, uint sequence, std::string pathname, std::string type){
    if (pathname == "/"){
        return -1;
    }
    if(pathname.size() > FS_MAXPATHNAME){
        return -1;
    }
    available_blocks_mutex.lock();
    if(blocks_full()){
        available_blocks_mutex.unlock();
        return -1;
    }
    available_blocks_mutex.unlock();
    vector<std::string> parsed_pathname; //parse filename on "/" so that we have each individual directory/filename
    if (split_string_spaces(parsed_pathname, pathname) == -1){
        return -1;
    }
    if(parsed_pathname.front().size() > FS_MAXFILENAME){ //If the file/directory name you want to create is too long, just ignore it
        return -1;
    }
    
    fs_inode curr_inode; //Start at root_inode, but this will keep track of which inode we're currently looking at

    path_lock parent_lock("/", parsed_pathname.size() > 1);
    
    disk_readblock(0, &curr_inode);
    fileserver_shared_lock.lock();
    string username = session_map[session].username;
    fileserver_shared_lock.unlock();

    //Uncomment to test concurrency for users being blocked (or not blocked) on shared pathname
    // if(username == "user1"){
    //     cout_lock.lock();
    //     cout << "user1 blocking" << endl;
    //     cout_lock.unlock();
    //     std::this_thread::sleep_for(std::chrono::seconds(90));
    //     cout_lock.lock();
    //     cout << "user1 finished blocking" << endl;
    //     cout_lock.unlock();
    // }
    // else{
    //     cout_lock.lock();
    //     cout << "user2 past user1 blocking" << endl;
    //     cout_lock.unlock();
    // }

    fs_direntry curr_entries[FS_DIRENTRIES]; //Will be an array of the direntries that are associated with the current inode
    for(size_t i = 0; i < FS_DIRENTRIES; ++i){ // initializes it all to 0 for now
        curr_entries[i].inode_block = 0;
    }
    int parent_inode_block = 0, parent_entries_block = 0;

    
    if(traverse_pathname_create(parsed_pathname, &curr_inode, curr_entries, parent_inode_block, parent_entries_block, username, parent_lock) == -1){
        return -1;
    }
    // if no room currently to add new direntry

    bool curr_entries_full = false;
    if(parent_entries_block == 0){
        parent_entries_block = add_block_to_inode(&curr_inode);
        if (parent_entries_block == -1){
            return -1;
        }
        for(size_t i = 0; i < FS_DIRENTRIES; ++i){ // initializes it all to 0 for now
            curr_entries[i].inode_block = 0;
            }
        curr_entries_full = true;
    }
    available_blocks_mutex.lock();
    if(blocks_full()){
        available_blocks_mutex.unlock();
        return -1;
    }
    

    //Create new inode and direntry for the file/directory we're creating
    fs_inode temp = create_inode(type, username);
    fs_direntry temp_entry;
    temp_entry.inode_block = *available_blocks.begin();
    available_blocks.erase(available_blocks.begin());
    available_blocks_mutex.unlock();
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
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    return password_map.find(query) != password_map.end();
}

string Fileserver::query_map(std::string query){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    return password_map[query];
}

bool Fileserver::query_session_map_sequence(uint session, uint sequence){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    return sequence <= session_map[session].sequence_num;
}

void Fileserver::insert_sequence(uint sequence, uint session){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    session_map[session].sequence_num = sequence;
}

string Fileserver::query_session_map_username(uint session){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    return session_map[session].username;
}

bool Fileserver::valid_session_range(uint session){
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    if(session_map.empty()){
        return true;
    }
    return session > session_map.size() - 1;
}

void Fileserver::init_fs() {
    //available_blocks.resize(FS_MAXFILEBLOCKS);
    lock_guard<mutex> fs_lock(available_blocks_mutex);
    for(size_t i = 1; i < FS_DISKSIZE; ++i){
        available_blocks.insert(i);
    }
    fs_inode curr_inode;
    fs_direntry curr_entries[FS_DIRENTRIES];
    size_t curr_ind;
    queue <size_t> matt;
    matt.push(0);
    while(!matt.empty()){
        curr_ind = matt.front();
        matt.pop();
        disk_readblock(curr_ind, &curr_inode);
        for (size_t i = 0; i < curr_inode.size; ++i){
            available_blocks.erase(available_blocks.find(curr_inode.blocks[i]));
            if(curr_inode.type == 'd'){
                disk_readblock(curr_inode.blocks[i], curr_entries);
                for(size_t j = 0; j < FS_DIRENTRIES; ++j){
                    if(curr_entries[j].inode_block != 0){
                        matt.push(curr_entries[j].inode_block);
                        available_blocks.erase(available_blocks.find(curr_entries[j].inode_block));
                    }
                }
            }
        }

    }
}

// - reads into entries array the directory at dir_inode[i]
// - can and probably will be called within a loop through dir_inode's blocks
// - entries is the table of dir_entries associated with the corresponding block value at i for dir_inode
// - whenever we create a file or directory, we need to update size and blocks in the root inode
void Fileserver::read_directory(fs_direntry *entries, fs_inode *dir_inode, size_t i) {
    lock_guard<mutex> fs_lock(fileserver_shared_lock);
    disk_readblock(dir_inode->blocks[i], &entries);
}