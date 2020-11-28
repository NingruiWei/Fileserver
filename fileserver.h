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
struct on_disk_lock{
    int lock_uses;
    shared_mutex lock;
};


class Fileserver{
    private:
        vector<session_map_entry> session_map;
        priority_queue<size_t, vector<size_t>, greater<size_t> > available_blocks;
        unordered_map<std::string, std::string> password_map;
        unordered_map<std::string, on_disk_lock> directory_lock_map;
        mutex fileserver_lock;
        
    public:
        Fileserver();
        ~Fileserver();
        bool blocks_full();
        int handle_fs_session(std::string session, std::string sequence, std::string username);
        int handle_fs_readblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type, char* readdata);
        int handle_fs_writeblock(std::string session, std::string sequence, std::string pathname, std::string block_or_type, char* data);
        int handle_fs_create(std::string session, std::string sequence, std::string pathname, std::string type);
        int handle_fs_delete(std::string session, std::string sequence, std::string pathname);
        void fill_password_map();
        bool username_in_map(std::string query);
        std::string query_map(std::string query);
        int query_session_map(int session);
        int valid_session_range();
        fs_inode get_curr_inode();
        void init_fs();
        void read_directory(fs_direntry *entries, fs_inode *dir_inode, size_t i);
        int traverse_pathname_delete(vector<std::string> &parsed_pathname, fs_inode* curr_inode, fs_direntry curr_entries[],
                                    int &curr_inode_block, int &parent_entries_block, fs_inode* parent_inode, int& parent_entry_index, int& parent_inode_block);
        int traverse_pathname(vector<std::string> &parsed_pathname, fs_inode* curr_inode, fs_direntry curr_entries[], int &parent_inode_block, int &parent_entries_block, bool type);
        void lock_on_disk(std::string path, bool shared_lock);
        void unlock_on_disk(std::string path, bool shared_lock);
        int add_block_to_inode(fs_inode* curr);

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

#endif 