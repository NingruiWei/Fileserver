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
    string username;
};

struct inode_plus{
    fs_direntry entries[FS_DIRENTRIES];
    fs_inode active_inode;
    mutex write_mutex;
    shared_mutex read_mutex;
};

struct File{
    priority_queue<int> available_blocks;
    vector<string> blocks;
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

class Fileserver{
    private:
        vector<File> files;
        vector<session_map_entry> session_map;
        vector<inode_plus> active_inodes;
        unordered_map<string, string> password_map;
        
    public:
        Fileserver();
        ~Fileserver();
        int handle_fs_session(string session, string sequence, string username);
        string handle_fs_readblock(string session, string sequence, string pathname, string block_or_type);
        void handle_fs_writeblock(string session, string sequence, string pathname, string block_or_type);
        void handle_fs_create(string session, string sequence, string pathname, string type);
        void handle_fs_delete(string session, string sequence, string pathname);
        void fill_password_map();
        bool username_in_map(string query);
        string query_map(string query);
        int query_session_map(int session);
        int valid_session_range();
        fs_inode get_curr_inode();
        void init_fs();
        void read_directory(fs_direntry *entries, fs_inode *dir_inode, size_t i);

};




























#endif 