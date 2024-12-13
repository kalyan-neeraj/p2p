#ifndef TRACKER_H
#define TRACKER_H

#include <string>
#include <netinet/in.h>
#include <vector>
#include<unordered_map>
#include <set>
using namespace std;


struct track_info {
    string ip;
    int port;
};

class Tracker {
public:
    struct chunkUser {
        unsigned long long number;
        string chunk;
        vector<string>username;
    };

struct ParsedData {
    string grpId;
    string username;
    string clientaddr;
    string file_name;
    };

struct UploadFile {
    string grpId;
    string username;
    vector<string>hashes;
    string fileName;
};


    int server_fd;
    int port;
    struct sockaddr_in ip_address;
    unordered_map<string, string>loginMap;
    unordered_map<string, bool>logintStatusMap;
    unordered_map<string, unordered_map<string, bool>>grpIdMap;
    unordered_map<string, set<string>>grp_fileMap;
    unordered_map<string, vector<chunkUser>>fileChunkUserMap;
    unordered_map<string, string>grpAdminMap;
    unordered_map<string, string>user_ipMap;
    unordered_map<int, string>client_userMap;
public:
    Tracker(track_info tf);
    ~Tracker();

    bool createUser(const std::string& user_name, const std::string& pass_word, string & user_address);
    void handleConnection(int user_socket);
    void start_tracker();
    bool  loggedIn(string& username);
    void logoutclient(int);
    string extractValue(const string& str, const string& key);
    void parseGroupData(const string& input);
    void updateUploads(UploadFile &uploadFile);
    void execute_download(string &name, string &fileName, string & grpId, int client_socket);
    void inputParser(string& token, string& commandName, vector<string>& params);
    void trim(string& s);
    string getWholeMessage(int client_socket);
};


#endif