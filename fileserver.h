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
        unordered_map<string, string> password_map;


    public:


        Fileserver();
        ~Fileserver();
        void fill_password_map();








};




























#endif 