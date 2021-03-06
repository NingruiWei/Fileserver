#include <iostream>
#include <cstdlib>
#include "fs_client.h"


using namespace std;

int main(int argc, char *argv[])

{
    char *server;
    int server_port;
    unsigned int session, seq=0;

    //const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

   // char readdata[FS_BLOCKSIZE];

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);
    unsigned int session1, seq1 = 0;
    cout << "reaching here" << endl;
    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;
    fs_session("user1", "password1", &session, seq++);
    fs_session("user2", "password2", &session1, seq1++);
    fs_create("user1", "password1", session, seq++, "/zeus", 'd');
    fs_create("user1", "password1", session, seq++, "/zeus", 'd');
    fs_create("user2", "password2", session1, seq1++, "/zeus/peeepee", 'd');
    fs_create("user1", "password1", session, seq++, "/paul", 'd');
    fs_create("user1", "password1", session, seq++, "/paul/klee", 'd');
    fs_create("user2", "password2", session, seq++, "/paul/klee/jean", 'd');
    fs_create("user1", "password1", session, seq++, "/paul/klee/kappa", 'd');
    fs_create("user1", "password1", session, seq++, "/paul/klee/jean/kappa", 'd');
    fs_create("user1", "password1", session, seq++, "/jake", 'd');
    fs_create("user1", "password1", session, seq++, "/jake1", 'd');
    fs_create("user1", "password1", session, seq++, "/jake2", 'd');
    fs_create("user1", "password1", session, seq++, "/jake3", 'd');
    fs_create("user1", "password1", session, seq++, "/jake4", 'd');
    fs_create("user1", "password1", session, seq++, "/jake5", 'd');
    fs_create("user1", "password1", session, seq++, "/jake6", 'd');
    fs_create("user1", "password1", session, seq++, "/jake7", 'd');
    fs_create("user1", "password1", session, seq++, "/jake7/paul", 'd');
    fs_create("user1", "password1", session, seq++, "/paul/klee/jean/jarjar", 'd');
    fs_create("user1", "password1", session, seq++, "/zeus", 'd');
    fs_create("user1", "password1", session, seq++, "/paul/klee/kappa", 'd');
    
    // fs_create("user1", "password1", session, seq++, "/paul/lion", 'd');
    // fs_create("user1", "password1", session, seq++, "/paul/lion/jira", 'f');
    //fs_writeblock("user1", "password1", session, seq++, "/paul/klee", 0, writedata);
    //fs_readblock("user1", "password1", session, seq++, "/paul/klee", 0, readdata);
    // fs_writeblock("user1", "password1", session, seq++, "/dir/file", 0, writedata);
    // fs_readblock("user1", "password1", session, seq++, "/dir/file", 0, readdata);
    // fs_delete("user1", "password1", session, seq++, "/dir/file");
    // fs_delete("user1", "password1", session, seq++, "/dir");
}