#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include<vector>
#include "fs_client.h"


using namespace std;

char *server;
int server_port;
const char *writedata1 = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
const char *writedata2 = "In 1901 she founded the Tuesday Musical Club in her home, with her sister Jenny Sachs and several other women, to play chamber music together and promote cultural events.[3] She was also founding president of the San Antonio chapter of the National Council of Jewish Women, organizer and president of the San Antonio Symphony Society, president of the Texas Free Kindergarten Association, and chair of the Texas committee for the Panama-Pacific International Exposition. From 1911 to 1913, she was president of the Texas Federation of Women's Clubs.[4][5] She oversaw the opening of a night school for immigrant workers in the city in 1907, and sewing classes for young women.[1] She was active in the city's Battle of Flowers parades for several years";
const char *writedata3 = "Tommy Black (born Fredrik Halldin) is a record producer from Sweden. He has worked with Kendrick Lamar, Schoolboy Q, Dr. Dre, Big Pooh, Ab-Soul and Shade Sheist. He was one of the producers of the 5 Grammy awarded album To Pimp A Butterfly by American rapper Kendrick Lamar. In 2017, Tommy Black received the award of SKAP - The Swedish Society of Songwriters, Composers and Authors.[1]";
const char *writedata4 = "The 1902 Ibrox disaster was the collapse of a stand at Ibrox Park (now Ibrox Stadium) in Glasgow, Scotland, which led to the deaths of 25 and injuries to more than 500 supporters during an international association football match between Scotland and England on 5 April 1902. The stadium had completed construction three years before the incident and was hosting its first international fixture, with the crowd estimated to be over 68,000. The match was the first time that the ground had been used more than half full. During the first half of the match, a section of the newly built West Tribune Stand collapsed, dropping between 200 and 300 people to the concrete floor below. Two spectators were declared dead at the scene, and a further twenty-three died of injuries sustained in the incident soon after, the last victim dying three weeks later.";
const char *writedata5 = "Frühschoppen (pronounced [fʁyːʃɔpm], meaning: an alcoholic drink before midday in company) is the German and Austrian tradition of meeting up at a pub, inn or tavern in the late morning, usually on Sundays. The specific customs vary from region to region. Frühschoppen is often a kind of brunch, but does not necessarily involve food. Frühschoppen is also often held at fun fairs, the most famous being Oktoberfest or Cannstatter Volksfest.";

vector<string> user1_files;
vector<string> user1_directories;
vector<string> user2_files;
vector<string> user2_directories;

void thread1(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for(int i = 0; i < 2; i++){
        string dir_name = "/dir" + to_string(i);
        fs_create("user1", "password1", session, seq++, dir_name.c_str(), 'd');
        user1_directories.push_back(dir_name);
        for(int j = 0; j < 10; j++){
            string file_name = dir_name + "/file" + to_string(j);
            fs_create("user1", "password1", session, seq++, file_name.c_str(), 'f');
            user1_files.push_back(file_name);
            for(int k = 0; k < 20; k++){
                fs_writeblock("user1", "password1", session, seq++, file_name.c_str(), k, writedata1);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    // fs_create("user1", "password1", session, seq++, "/user1_dir_lvl1", 'd');
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_create("user1", "password1", session, seq++, "/user1_file_lvl1", 'f');
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_writeblock("user1", "password1", session, seq++, "/user1_file_lvl1", 0, writedata1);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_readblock("user1", "password1", session, seq++, "/user1_file_lvl1", 0, readdata);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // cout << "Thread 1: " << string(readdata, 512) << endl;

    // fs_delete("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/dir_lvl4/dir_lvl5");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread2(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user2", "password2", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for(int i = 0; i < 2; i++){
        string dir_name = "/dir" + to_string(i);
        fs_create("user2", "password2", session, seq++, dir_name.c_str(), 'd');
        user2_directories.push_back(dir_name);
        for(int j = 0; j < 10; j++){
            string file_name = dir_name + "/file" + to_string(j);
            fs_create("user2", "password2", session, seq++, file_name.c_str(), 'f');
            user2_files.push_back(file_name);
            for(int k = 0; k < 20; k++){
                fs_writeblock("user2", "password2", session, seq++, file_name.c_str(), k, writedata2);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    // fs_create("user2", "password2", session, seq++, "/user2_dir_lvl1", 'd');
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_create("user2", "password2", session, seq++, "/user2_file_lvl1", 'f');
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_writeblock("user2", "password2", session, seq++, "/user2_file_lvl1", 0, writedata2);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_readblock("user2", "password2", session, seq++, "/user2_file_lvl1", 0, readdata);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // cout << "Thread 2: " << string(readdata, 512) << endl;

    // fs_delete("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_delete("user2", "password2", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/dir_lvl4");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread3(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_dir_lvl2", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2", 0, writedata3);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 3: " << string(readdata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/user1_file_lvl1");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread4(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;

    fs_session("user2", "password2", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_dir_lvl2", 'd');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_create("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2", 'f');
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_writeblock("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2", 0, writedata4);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 4: " << string(readdata, 512) << endl;

    fs_delete("user2", "password2", session, seq++, "/user2_file_lvl1");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_delete("user2", "password2", session, seq++, "/dir_lvl1/dir_lvl2");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void thread5(int val){
    char readdata[FS_BLOCKSIZE];
    unsigned int session, seq=0;
    unsigned int other_session, other_seq = 0;

    fs_session("user1", "password1", &session, seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_session("user2", "password2", &other_session, other_seq++);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // fs_create("user3", "password3", session, seq++, "/user3_dir_lvl1", 'd');
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // fs_create("user3", "password3", session, seq++, "/user3_file_lvl1", 'f');
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    fs_writeblock("user1", "password1", session, seq++, "/user1_file_lvl1", 0, writedata5);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/user1_file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 5 Part 1: " << string(readdata, 512) << endl;

    fs_writeblock("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2", 0, writedata5);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_file_lvl2", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 5 Part 2: " << string(readdata, 512) << endl;

    fs_writeblock("user2", "password2", session, seq++, "/user2_file_lvl1", 0, writedata5);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user2", "password2", session, seq++, "/user2_file_lvl1", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 5 Part 3: " << string(readdata, 512) << endl;

    fs_writeblock("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2", 0, writedata5);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_readblock("user2", "password2", session, seq++, "/user2_dir_lvl1/user2_file_lvl2", 0, readdata);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "Thread 5 Part 4: " << string(readdata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/user1_dir_lvl1/user1_dir_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_delete("user2", "password2", other_session, other_seq++, "/user2_dir_lvl1/user2_dir_lvl2");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_delete("user1", "password1", session, seq++, "/user1_dir_lvl1");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs_delete("user2", "password2", other_session, other_seq++, "/user2_dir_lvl1");
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
    thread t2(thread2, 0);
    //thread t3(thread3, 0);
    //thread t4(thread4, 0);
    //thread t5(thread5, 0);

    t1.join();
    t2.join();
    //t3.join();
    //t4.join();
    //t5.join();

    return 0;
}