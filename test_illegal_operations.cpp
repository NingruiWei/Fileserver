#include <iostream>
#include <cstdlib>
#include "fs_client.h"


using namespace std;

int main(int argc, char *argv[])

{
    char *server;
    int server_port;
    unsigned int user1_session, user1_seq=0;
    unsigned int user2_session, user2_seq = 0;
    unsigned int user2_session2, user2_seq2 = 0;

    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    const char *writedata2 = "Uruguayans (Spanish: uruguayos) are people identified with the country of Uruguay, through citizenship or descent. Uruguay is home to people of different ethnic origins. As a result, many Uruguayans do not equate their nationality with ethnicity, but with citizenship and their allegiance to Uruguay. Colloquially, primarily among other Spanish-speaking Latin American nations, Uruguayans are also referred to as ";

    char readdata[FS_BLOCKSIZE];

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    cout << "reaching here" << endl;
    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;
    //Invalid username and password pair
    fs_session("user1", "password3", &user1_session, user1_seq++);

    fs_session("user1", "password1", &user1_session, user1_seq++);
    fs_session("user2", "password2", &user2_session, user2_seq++);
    fs_session("user2", "password2", &user2_session2, user2_seq2++);
    
    //Shouldn't be able to create a directory or file under someone else's session
    fs_create("user2", "password2", user1_session, user1_seq++, "/wrond_session_num", 'd');

    fs_create("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir", 'd');
    fs_create("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file", 'f');

    //Cannot create using invalid session or sequence nums
    fs_create("user2", "password2", 1234, user2_seq++, "/invalid_session_num", 'd');
    fs_create("user2", "password2", user2_session2, 1, "/invalid_sequence_num", 'd');

    //Shouldn't be able to write data to a directory
    fs_writeblock("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir", 0, writedata);

    fs_writeblock("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file", 0, writedata);
    fs_readblock("user2", "password2", user2_session2, user2_seq2++, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should be writedata

    fs_writeblock("user2", "password2", user2_session2, user2_seq2++, "/right_session_num_dir/file", 0, writedata2);
    fs_readblock("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should be writedata2

    //Cannot write or read with wrong session number
    fs_writeblock("user2", "password2", 1234, user2_seq2++, "/right_session_num_dir/file", 0, writedata);
    fs_readblock("user2", "password2", 1234, user2_seq++, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should be writedata2

    //Cannot write or read with wrong sequence number
    fs_writeblock("user2", "password2", user2_session2, 1, "/right_session_num_dir/file", 0, writedata);
    fs_readblock("user2", "password2", user2_session, 1, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should be writedata2

    //Incorrect user cannot write or read a file/directory that does not belong to it
    fs_writeblock("user1", "password1", user1_session, user1_seq++, "/right_session_num_dir/file", 0, writedata);
    fs_readblock("user1", "password1", user1_session, user1_seq++, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should still be writedata2

    //Cannot delete another user's file/directory
    fs_delete("user1", "password1", user1_session, user1_seq++, "/right_session_num_dir");

    //Cannot delete a directory with contents within it
    fs_delete("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir");

    //Cannot delete with wrong session or sequence number
    fs_delete("user2", "password2", 1234, user2_seq++, "/right_session_num_dir/file");
    fs_delete("user2", "password2", user2_session, 1, "/right_session_num_dir/file");

    fs_delete("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file");
    fs_delete("user2", "password2", user2_session2, user2_seq2++, "/right_session_num_dir");

    //Cannot create to a directory that no longer exists
    fs_create("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file", 'f');

    //Cannot read or write to a file that no longer exists
    fs_writeblock("user2", "password2", user2_session, user2_seq++, "/right_session_num_dir/file", 0, writedata);
    fs_readblock("user2", "password2", user2_session2, user2_seq2++, "/right_session_num_dir/file", 0, readdata);
    cout << string(readdata, 512) << endl; //Read data should still be writedata2

    //Cannot delete the root directory
    fs_delete("user1", "password1", user1_session, user1_seq++, "/");


    return 0;
}