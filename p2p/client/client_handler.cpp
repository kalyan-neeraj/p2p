#include <vector>
#include <iostream>
#include <string>
#include "user.cpp"

using namespace std;

int main(int argc, char* argv[]) {
    vector<string> params;
    if (argc != 3) {
        cout << "Invalid arguments\n";
        return 0;
    }
    params.push_back(argv[1]);
    params.push_back(argv[2]);
    Client client(params);
    client.connectToTracker();
    thread listener_thread(&Client::listen_for_requests, &client);
    while (true) {
        string token;
        getline(cin, token);
        client.execute_command(token);
    }
    listener_thread.join();

    return 0;
}
