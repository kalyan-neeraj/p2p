#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <netinet/in.h>
#include <vector>

using namespace std;

struct tracker_info {
    int port;
    string ip;
};

struct Input
{
    int port;
    string ip;
    string filepath;
};



vector<tracker_info> getTrackerInfo(string & filepath);
string getFileName(string & filePath);
void getInput(Input & input, vector<string>& params);


#endif