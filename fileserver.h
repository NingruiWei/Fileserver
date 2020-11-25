#ifndef _FILESERVER_H_
#define _FILESERVER_H_
#include <sys/types.h>
#include <cstdint>
#include "fs_param.h"
#include "fs_crypt.h"
#include "fs_server.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <algorithm>
using namespace std;


struct session_map_entry{
    size_t sequence_num;
    std::string username;
};

struct inode_plus{
    fs_direntry entries[FS_DIRENTRIES];
    fs_inode active_inode;
    mutex write_mutex;
    shared_mutex read_mutex;
};

struct File{
    priority_queue<int> available_blocks;
    vector<std::string> blocks;
    File(){ 
        // read the root inode and determine which blocks are free within root.blocks (spec 6.2)
        for(size_t i = 0; i < FS_MAXFILEBLOCKS; ++i){
            available_blocks.push(i);
        }
        blocks.resize(FS_MAXFILEBLOCKS);
    }

    bool is_full(){
        return available_blocks.empty();
    }

};

struct path_lock{
    std::string pathname;
    bool shared_lock;
    Fileserver *running_fs;

    path_lock(std::string path, bool shared_status, Fileserver *current_server){
        pathname = path;
        shared_lock = shared_status;
        running_fs = current_server;
        running_fs->lock_on_disk(pathname, shared_lock);
    }

    //RAII unlock
    ~path_lock(){
        running_fs->unlock_on_disk(pathname, shared_lock);
    }

    //Hand-over-hand locking so that we can easily switch locks with one another.
    void swap_lock(path_lock *swap_to){
        swap(this->shared_lock, swap_to->shared_lock);
        swap(this->pathname, swap_to->pathname);
    }
        
};

struct on_disk_lock{
    int lock_uses;
    shared_mutex lock;
};

class Fileserver{
    private:
        vector<File> files;
        vector<session_map_entry> session_map;
        vector<inode_plus> active_inodes;
        priority_queue<size_t> available_blocks;
        unordered_map<std::string, std::string> password_map;
        unordered_map<std::string, on_disk_lock> directory_lock_map;
        mutex filerserver_lock;
        
    public:
        Fileserver();
        ~Fileserver();
        bool blocks_full();
        int handle_fs_session(std::string session, std::string sequence, std::string username);
        std::string handle_fs_readblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type);
        void handle_fs_writeblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type);
        void handle_fs_create(std::string session, std::string sequence, std::string pathname, std::string type);
        void handle_fs_delete(std::string session, std::string sequence, std::string pathname);
        void fill_password_map();
        bool username_in_map(std::string query);
        std::string query_map(std::string query);
        int query_session_map(int session);
        int valid_session_range();
        fs_inode get_curr_inode();
        void init_fs();
        void read_directory(fs_direntry *entries, fs_inode *dir_inode, size_t i);
        int traverse_pathname(vector<std::string> &parsed_pathname, fs_inode* curr_inode, fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block);
        int traverse_single_file(std::string desired_file, fs_inode* curr_inode, fs_direntry curr_entires[]);
        void lock_on_disk(std::string path, bool shared_lock);
        void unlock_on_disk(std::string path, bool shared_lock);

};
#endif 