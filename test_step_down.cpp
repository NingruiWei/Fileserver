#include <iostream>
#include <cstdlib>
#include "fs_client.h"


using namespace std;

int main(int argc, char *argv[])

{
    char *server;
    int server_port;
    unsigned int session, seq=0;

    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    const char *writedata1 = "Los Toldos (Sometimes referred to as General Viamonte) is a small town in Buenos Aires Province, Argentina, situated in General Viamonte Partido, which developed around a station of the same name on the Buenos Aires Western Railway. It is located at 310 km (193 mi) from the city of Buenos Aires and had about 17,600 inhabitants in 2001.";
    const char *writedata2 = "Kalpakkam is a town in Tamil Nadu, India, situated on the Coromandel Coast 70 kilometres south of Chennai. A conglomerate of two villages (Puduppattinam and Sadurangappatinam) and a DAE township, it is about 55 kilometres (34 mi) from Thiruvanmiyur and 85 kilometres (53 mi) from Pondicherry. This coastal town is humid. Summers here prevail from early March till late May. Temperatures in the Summer vary from 32 degrees Celsius and can go up to 41 degrees Celsius.";
    const char *writedata3 = "The Canadian women's national cricket team is the team that represents the country of Canada in international women's cricket matches. They made their international debut in September 2006 in a three match series of one-day games against Bermuda to decide which team would represent the Americas region in the Women's Cricket World Cup Qualifier in Ireland in 2007. Canada started well, with a five wicket win in the first win - Game 1,[3] but Bermuda came back with 24 run win in the second - Game 2.[4] The third game went down to the wire, with Bermuda triumphing by just 3 runs - Game 3";

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
    fs_session("user1", "password1", &session, seq++);
    fs_create("user1", "password1", session, seq++, "/dir_lvl1", 'd');
    fs_create("user1", "password1", session, seq++, "/file_lvl1", 'f');

    fs_create("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2", 'd');
    fs_create("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2", 'f');

    fs_create("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3", 'd');
    fs_create("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/file_lvl3", 'f');

    fs_create("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/dir_lvl4", 'd');
    fs_create("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/file_lvl4", 'f');

    fs_writeblock("user1", "password1", session, seq++, "/file_lvl1", 0, writedata);
    fs_readblock("user1", "password1", session, seq++, "/file_lvl1", 0, readdata);
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_writeblock("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2", 0, writedata1);
    fs_readblock("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2", 0, readdata);
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_writeblock("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/file_lvl3", 0, writedata2);
    fs_readblock("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/file_lvl3", 0, readdata);
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_writeblock("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/file_lvl4", 0, writedata3);
    fs_readblock("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/file_lvl4", 0, readdata);
    cout << "lvl1 " << string(writedata, 512) << endl;

    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/file_lvl2");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/file_lvl4");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3/dir_lvl4");
    fs_delete("user1", "password1", session, seq++, "/file_lvl1");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/dir_lvl3");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2/file_lvl3");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1/dir_lvl2");
    fs_delete("user1", "password1", session, seq++, "/dir_lvl1");
    return 0;
}