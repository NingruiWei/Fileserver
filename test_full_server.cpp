#include <iostream>
#include <cstdlib>
#include "fs_client.h"


using namespace std;

int main(int argc, char *argv[])
{
    char *server;
    int server_port;
    unsigned int session, seq=0;

    const char *data = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    char readdata[FS_BLOCKSIZE];

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);
    fs_session("user1", "password1", &session, seq++);

    //Create 2 Directories, each with 33 files each filled to the max (plus 1 just to make sure it won't add beyond the allotted amount of blocks in a file)
    //The 1st directory will be able to be filled completely, but leaves only 1 block for directory 2, so directory 2 shouldn't have any files or data in it
    for(int i = 0; i < 2; i++){
        string directory_name = "/directory" + to_string(i);
        fs_create("user1", "password1", session, seq++, directory_name.c_str(), 'd'); 
        for(int j = 0; j < 33; j++){
            string file_name = directory_name + "/file" + to_string(j);
            fs_create("user1", "password1", session, seq++, file_name.c_str(), 'f');
            for(int k = 0; k < 124; k++){
                fs_writeblock("user1", "password1", session, seq++, file_name.c_str(), k, data);
            }
        }
    }

    fs_readblock("user1", "password1", session, seq++, "/directory1/file0", 0, readdata);
    cout << "Directory 1 File 0 Block 0 output: " << readdata << endl; //Should be empty

    fs_readblock("user1", "password1", session, seq++, "/directory1/file0", 123, readdata);
    cout << "Directory 1 File 0 Block 123 output: " << readdata << endl; //Should be empty

    fs_readblock("user1", "password1", session, seq++, "/directory0/file0", 0, readdata);
    cout << "Directory 0 File 0 Block 0 output: " << readdata << endl;

    fs_readblock("user1", "password1", session, seq++, "/directory0/file0", 123, readdata);
    cout << "Directory 0 File 0 Block 123 output: " << readdata << endl;

    fs_readblock("user1", "password1", session, seq++, "/directory0/file31", 0, readdata);
    cout << "Directory 0 File 31 Block 0 output: " << readdata << endl;

    //This last read causes an assertion on a disk_readwrite due to being passed a block too big while on the path to find the correct block to read
    fs_readblock("user1", "password1", session, seq++, "/directory0/file31", 123, readdata);
    cout << "Directory 0 File 31 Block 123 output: " << readdata << endl;

    return 0;
}