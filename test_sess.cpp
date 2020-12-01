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
    //char readdata[FS_BLOCKSIZE];

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    unsigned int user1_session = session;
    unsigned int user1_seq = seq;
    unsigned int user2_session = session;
    unsigned int user2_seq = seq;

    fs_clientinit(server, server_port);
    fs_create("user1", "password1", user1_session, user1_seq++, "/home", 'd');
    fs_create("user2", "password2", user2_session, user2_seq++, "/genshin", 'f');
    user1_session = 0;
    fs_session("user1", "password1", &user1_session, user1_seq++);
    user1_session++;
    fs_create("user1", "password1", user1_session, user1_seq++, "/impact", 'd');
    user2_session = 0;
    fs_session("user2", "password2", &user1_session, user2_seq++);
    user2_session++;
    cout << "CHANGING USER1 SESSION & SEQUENCE TO USER2 SESSION & SEQUENCE" << endl;
    user1_session = user2_session;
    user1_seq = user2_seq;
    fs_create("user1", "password1", user1_session, user1_seq++, "/impact", 'd');
    fs_session("user1", "password2", &user1_session, user1_seq++);
    fs_session("user2", "password1", &user2_session, user2_seq++);
    fs_session("asdawd2rawdwasdaw", "password1", &user1_session, user1_seq++);
    fs_session("user1", "asdawdwadwaawwdawdawdd", &user1_session, user1_seq++);
    fs_session("wdawadwdawdwa", "pawadw2addwad", &session, seq++);
    return 0;
    
}