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
using namespace std;
struct File{
    priority_queue<int> available_blocks;
    vector<string> blocks;
    File(){ 
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
        vector<size_t> session_map;
        unordered_map<string, string> password_map;
        
        


    public:


        Fileserver();
        ~Fileserver();
        int handle_fs_session(string session, string sequence);
        string handle_fs_readblock(string session, string sequence, string pathname, string block_or_type);
        void handle_fs_writeblock(string session, string sequence, string pathname, string block_or_type);
        void handle_fs_create(string session, string sequence, string pathname);
        void handle_fs_delete(string session, string sequence, string pathname);
        void fill_password_map();
        bool username_in_map(string query);
        string query_map(string query);
        int query_session_map(int session);
        int valid_session_range();








};




























#endif 