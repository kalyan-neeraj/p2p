#include "user.h"
#include <fcntl.h>
#include <string>
#include <cstring>
#include <unistd.h> 
#include <iostream>
#include <sstream>


using namespace std;

void setTrackerInfo(vector<tracker_info>& tracker_inf, const string& tracker) {
    istringstream iss(tracker);
    string entry;
    
    while (getline(iss, entry)) {
        size_t colon_pos = entry.find_last_of(':');
        if (colon_pos != string::npos) {
            tracker_info info;
            info.ip = entry.substr(0, colon_pos);
            info.port = stoi(entry.substr(colon_pos + 1));
            tracker_inf.push_back(info);
        }
    }
}

vector<tracker_info> getTrackerInfo(string& filePath) {

    string tracker_info_path = filePath;
    int fd = open(tracker_info_path.c_str(), O_RDONLY, 0644);
    if (fd < 0) {
        cout<<"Error in opening file\n";
        exit(EXIT_FAILURE);
    }
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

     for (int i = 0; i < file_size; i++) {
        unsigned long long bytesRead = read(fd, &buffer[i], 1);
    }

    buffer[file_size] = '\0';
    close(fd);

    string tracker = "";
    int start = 0;
    while (buffer[start] != '\0') {
        tracker += buffer[start];
        start++;
    }

    vector<tracker_info> tracker_inf;
    setTrackerInfo(tracker_inf, tracker);
    return tracker_inf;
}



string getFileName(string &file_path) {
    if (file_path.back() == '/') {
        file_path.pop_back();
    }
    unsigned long long last_slash_pos = file_path.find_last_of('/');

    if (last_slash_pos == string::npos) {
        return file_path;
    }

    return file_path.substr(last_slash_pos + 1);
}

void setIpandPort(Input& input,  string& s) {
    size_t colonPos = s.find(':');
    if (colonPos == string::npos || colonPos == 0 || colonPos == s.length() - 1) {
        cout << "Invalid Input: missing or misplaced colon\n";
        return;
    }

    input.ip = s.substr(0, colonPos);

    try {
        input.port = stoi(s.substr(colonPos + 1));
        if (input.port < 0 || input.port > 65535) {
            throw out_of_range("Port number out of valid range");
        }
    } catch ( exception& e) {
        cout << "Invalid Input: " << e.what() << "\n";
        input.ip.clear();
        input.port = -1;
    }
}

void getInput(Input& input, vector<string>& params) {
    string s = params[0];
    input.filepath = params[1];
    setIpandPort(input, s);
}