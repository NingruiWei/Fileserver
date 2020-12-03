#include <iostream>
#include <cstdlib>
#include <string>
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
    cout << "clientinit finished" << endl;
    fs_session("user1", "password1", &session, seq++);
    fs_readblock("user1", "password1", session, seq++, "/zeus", 0, readdata); //Read file that has yet to be initalized
    cout << "Invalid read: " << string(readdata, 512) << endl;
    fs_create("user1", "password1", session, seq++, "/zeus", 'f');
    fs_readblock("user1", "password1", session, seq++, "/zeus", 0, readdata); //Read block that has yet to be initalized
    cout << "Uninitalized read: " << string(readdata, 512) << endl;

    fs_writeblock("user1", "password1", session, seq++, "/zeus", 0, data);
    fs_readblock("user1", "password1", session, seq++, "/zeus", 0, readdata); //Read block after initilaziation
    cout << "valid read: " << string(readdata, 512) << endl;

    fs_readblock("user1", "password1", session, seq++, "/zeus", 100000, readdata); //Read block outside of possible range of blocks

    fs_delete("user1", "password1", session, seq++, "/zeus");

    fs_create("user1", "password1", session, seq++, "/zeus", 'f');
    fs_writeblock("user1", "password1", session, seq++, "/zeus", 0, data);

    fs_readblock("user1", "password1", session, seq++, "zeus", 0, readdata);
    fs_readblock("user1", "password1", session, seq++, "/zeus1", 0, readdata);
    fs_readblock("user1", "password1", session, seq++, "/zeus/poseidon", 0, readdata);

    //This causes too much output for the AG
    // for(int i = 0; i < (8*124); i++){
    //     std::string create_me = "/dir" + to_string(i);
    //     fs_create("user1", "password1", session, seq++, create_me.c_str(), 'd');
    // }

    // fs_create("user1", "password1", session, seq++, "/one_more", 'f');

    return 0;
}