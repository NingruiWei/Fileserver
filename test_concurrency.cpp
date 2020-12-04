#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "fs_client.h"


using namespace std;

char *server;
int server_port;
const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";


void thread1(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/I have spaces", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "null_\0_character", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread2(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/I have spaces", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "null_\0_character", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread3(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/I have spaces", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "null_\0_character", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread4(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/I have spaces", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "null_\0_character", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread5(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/I have spaces", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "null_\0_character", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char *argv[])
{

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;

    thread t1(thread1, 0);
    //thread t2(thread2, 0);
    //thread t3(thread3, 0);
    //thread t4(thread4, 0);
    //thread t5(thread5, 0);

    t1.join();
    //t2.join();
    //t3.join();
    //t4.join();
    //t5.join();

    return 0;
}