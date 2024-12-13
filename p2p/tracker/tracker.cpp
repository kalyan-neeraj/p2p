#include "tracker.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <stdexcept>
#include <fcntl.h>
#include <arpa/inet.h>
#include <algorithm>
#include <thread>
#include <sstream>

using namespace  std;

Tracker::Tracker(track_info tf) {
   this->port = tf.port;
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd < 0) {
        throw runtime_error("Failed to create socket");
    }

    this->ip_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, tf.ip.c_str(), &(this->ip_address.sin_addr)) <= 0) {
    throw runtime_error("Invalid IP address");
}
    this->ip_address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&ip_address, sizeof(ip_address)) < 0) {
        cerr << "Bind failed: " << strerror(errno) << endl;
        throw runtime_error("Failed to bind socket");
    }

    if (listen(server_fd, 3) < 0) {
        throw runtime_error("Failed to listen on socket");
    }
}


Tracker::~Tracker() {
    close(server_fd);
}


/************************************************************************************************************************************************************************/


void Tracker::trim(string& str) {
    if (str.empty()) return;

    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\0' || std::isspace(str[start]))) {
        ++start;
    }

    if (start == str.length()) {
        str.clear();
        return;
    }

    size_t end = str.length() - 1;
    while (end > start && (str[end] == ' ' || str[end] == '\0' || std::isspace(str[end]))) {
        --end;
    }
    str = str.substr(start, end - start + 1);
}


void Tracker::updateUploads(UploadFile& uploadFile) {
    string filename = uploadFile.fileName;
    string grpId = uploadFile.grpId;
    string  user = uploadFile.username;
    vector<string>hsh;

    for (auto it : uploadFile.hashes) {
        trim(it);
        hsh.emplace_back(it);
    }

    trim(grpId);
    trim(user);
    trim(filename);

    cout<<grpId<<" "<<grpId.size()<<"\n";
     cout<<user<<" "<<user.size()<<"\n";
      cout<<filename<<" "<<filename.size()<<"\n";
     grp_fileMap[grpId].insert(filename);
    vector<chunkUser> cusers;
    cout<<"Starting Upload......\n";
    string key = grpId+ ":" + filename;
    unsigned long long i = 0;
    for (auto it : hsh) {
        chunkUser cu;
        cu.chunk = it;
        cu.number = i;
        cu.username.emplace_back(user);
        fileChunkUserMap[key].emplace_back(cu);
        i++;
    }
    cout<<fileChunkUserMap[key].size()<<"\n";
    cout<<"Done Upload......\n";
}

string Tracker::extractValue(const string& str, const string& key) {
    size_t pos = str.find("\"" + key + "\":\"");
    if (pos != string::npos) {
        pos += key.length() + 4; // Move past "key":"
        size_t endPos = str.find("\"", pos);
        if (endPos != string::npos) {
            return str.substr(pos, endPos - pos);
        }
    }
    return "";
}

/***************************************************************************************************************************************************************************** */

bool Tracker::createUser(const string& user_name, const string& pass_word, string & user_address) {
    if (loginMap.find(user_name) != loginMap.end()) {
        cout << "User: " << user_name << " already exists\n";
        return false;
    } else {
        // Add the new user to the loginMap
        cout<<pass_word;
        loginMap[user_name] = pass_word;
        logintStatusMap[user_name] = false;
        user_ipMap[user_name] = user_address;
        cout << "User: " << user_name << " created successfully\n";
        return true;
    }
}

void Tracker::logoutclient(int client_socket) {
    string username = client_userMap[client_socket];
    logintStatusMap[username] = false;
    client_userMap.erase(client_socket);
    close(client_socket);
}

void Tracker::inputParser(string& token, string& commandName, vector<string>& params) {
   int token_length = token.size();
   cout<<"TOken recieved: "<<token<<endl;
    int new_start = 0;
    int cnt = 0;

    for (int i = 0; i <= token_length; i++) {
        if (i == token_length || token[i] == ' ') {
            if (i > new_start) {
                if (cnt == 0) {
                    commandName = token.substr(new_start, i - new_start);
                    cnt++;
                } else {
                    string str = token.substr(new_start, i - new_start);
                    trim(str);
                    params.push_back(str);
                }
            }
            new_start = i + 1;
        }
    }
}

bool Tracker::loggedIn(string& username) {
    auto it = logintStatusMap.find(username);
    if (it == logintStatusMap.end() || logintStatusMap[username] == false) {
        return false;
    }
    return true;
}


void Tracker::execute_download(string &name, string &fileName, string &grpId, int client_socket) {
    cout<<"At execute download\n";
    string response = "";
    cout<<fileName<<"\n";
    cout<<grpId<<"\n";
    cout<<grpId.size()<<"\n";
    if (loginMap.find(name) == loginMap.end()) {
            response = "User not logged In\n";
    } else if (grpIdMap.find(grpId) == grpIdMap.end()) {
        response = "Group not found\n";
    } else if (grp_fileMap[grpId].find(fileName) == grp_fileMap[grpId].end()) {
        response = "File with " + fileName + " Not found\n";
    } else {
        string chunks = "";
        string user = "";
        string file_key = grpId + ":" + fileName;
        for (auto it : fileChunkUserMap[file_key]) {
            for (const auto& user : it.username) {
                if (user != name && user_ipMap.find(user) != user_ipMap.end()) {
                    string ip_port = user_ipMap[user];
                    response += (it.chunk +"_"+ to_string(it.number)+ ":" + ip_port + " ");
                }
                response +="\n";
            }
        }
    }
    send(client_socket, response.c_str(), response.size()+1, 0);
}

string Tracker::getWholeMessage(int client_socket) {
    vector<char> token;
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    cout << "Started....Reading from Client\n";
    long long valrecv;
    while ((valrecv = recv(client_socket, buffer, buffer_size, 0)) > 0) {
        buffer[valrecv] = '\0';
        token.insert(token.end(), buffer, buffer + valrecv);
        if (strchr(buffer, '\0') != nullptr) {
            break;
        }
    }

    if (valrecv == 0) {
        logoutclient(client_socket);
        cout << "Client disconnected\n";
    } else if (valrecv < 0) {
        cerr << "Failed to recv from socket: " << strerror(errno) << endl;
    }

    cout << "Ended....Reading from Client\n";
    return string(token.begin(), token.end());
}


void Tracker::handleConnection(int client_socket) {

    while (true) {
    // Read the full message from the client
   
    string received_message = getWholeMessage(client_socket);
    if (received_message.empty()) {
        logoutclient(client_socket);
        return;
    }

    // Parse the message using inputParser
    cout<<"Recieved: "<<received_message<<"\n";
    string commandName;
    vector<string> params;
    inputParser(received_message, commandName, params);

    trim(commandName);
    cout<<"Command: "<<commandName<<" Size : "<<commandName.size()<<"\n";

    // Check if command is CREATE_USER
    if (commandName == "create_user") {
        if (params.size() == 3) {
            string username = params[0];
            string password = params[1];
            string user_address = params[2];

            if (loginMap.find(username) != loginMap.end()) {
                string res = "User already exists\n";
                send(client_socket, res.c_str(), res.size()+1, 0); 
            } else if (logintStatusMap.find(username) != logintStatusMap.end()) {
                string res = "User logged in exists\n";
                send(client_socket, res.c_str(), res.size()+1, 0); 
            }
            else {
            // Call the createUser function to handle this logic
            if (createUser(username, password, user_address)) {
                // Send success response back to the client
                string response = "create_user:" + username + " " + password + " " ;
                send(client_socket, response.c_str(), response.size()+1, 0);
            } else {
                // User already exists, send an error response
                string error_response = "Error: User " + username + " already exists" ;
                send(client_socket, "NO", 0, 0);
            }
            }
            }
         else {
            // If there aren't enough parameters, send an error response
            string error_response = "Error: Not enough parameters for CREATE_USER" ;
            send(client_socket, "NO", 0, 0);
        }
        }

    else if (commandName == "login") {
        if (params.size() == 2) {
            string user_name = params[0];
            string pass_word = params[1];
            cout<<user_name<<"\n";
            // cout<<pass_word<<"\n";

            auto it = loginMap.find(user_name);
            cout<<loginMap[user_name].size()<<"\n";
            if (loggedIn(user_name)) {
                cout<<"User already logged in \n";
                send(client_socket, "login", 9, 0);
               
            } else if (it == loginMap.end() || loginMap[user_name] != pass_word) {
                cout<<"Invalid credentials"<<"\n";
               send(client_socket, "NO", 2, 0);
            } else {
                logintStatusMap[user_name] = true;
                client_userMap[client_socket] = user_name;
                cout<<"User logged in successfully\n";
                send(client_socket, "login true", 10, 0);
            }
        }
    }

    else if (commandName == "create_group") {
        cout<<"IM here\n";
            if (params.size() != 2) {
                cout<<"Invalid params\n";
                 send(client_socket, "NO", 2, 0);
               
            } else {
                string grp_id = params[0];
                string username = params[1];
                trim(grp_id);
                trim(username);

                if (!loggedIn(username)) {
                    string response = "User has to be logged In";
                    send(client_socket, response.c_str(), response.size()+1, 0);
                }

                else {
                    if (grpIdMap.find(grp_id) != grpIdMap.end()) {
                    string response = "group with id :"+grp_id + " already exits" + "\n";
                    cout<<"group with id :"<<grp_id<<" already exits"<<"\n";
                     send(client_socket, response.c_str(), response.size()+1, 0);
                   
                } else {
                        //setting the owner//
                       grpIdMap[grp_id][username] = true;
                       grpAdminMap[grp_id] = username;
                       string response = "group with id :" +grp_id+ " created successfully";
                       cout<<response<<endl;
                    send(client_socket, response.c_str(), response.size()+1, 0);     
                }

                }

            }
        } 

        else if (commandName == "join_group") {
            if (params.size() != 2) {
                cout<<"Invalid params\n";
                 send(client_socket, "NO", 2, 0);
               
            } else {
                string grp_id = params[0];
                string username = params[1];

                if (!loggedIn(username)) {
                    string response = "User has to loggedIn\n";
                    send(client_socket, response.c_str(), response.size()+1, 0);
                } else {
                     if (grpIdMap.find(grp_id) == grpIdMap.end()) {
                      string res = "group with id :" + grp_id + " doesn't exist"+"\n";
                    cout<<"group with id :"<<grp_id<<" doesn't exist"<<"\n";
                     send(client_socket, res.c_str(), res.size()+1, 0);
                   
                } else if (username == grpAdminMap[grp_id] ) {
                    string res = "YOU ARE THE ADMIN\n";
                    send(client_socket, res.c_str(), res.size()+1, 0);
                }
                
                
                else {
                       grpIdMap[grp_id][username] = false;
                       cout<<"sent request group with id :"<<grp_id<<" successfully"<<"\n";
                       string res = "sent request group with id : " + grp_id+" successfully\n"; 
                        send(client_socket, res.c_str(), res.size()+1, 0);     
                }

                }

               
            }
        }

        else if (commandName == "list_groups") {
            cout<<"IM here\n";
            string s = "List of Groups\n";
            for (const auto& it : grpIdMap) {
                string res = it.first + "\n";
                s+= res;
            
            }
            send(client_socket, s.c_str(), s.size()+1, 0);
        }

        else if (commandName == "accept_request") {
            if (params.size() != 3) {
                cout<<"Invalid params"<<"\n";
            }

            for (auto it : params) {
                cout<<it<<" ";
            }

            string grp_id = params[0];
            string user_name = params[1];
            string res = "";

            if (grpIdMap.find(grp_id) == grpIdMap.end()) {
                res = "Invalid group id\n";
                cout<<"Invalid group id\n";
            } else if (grpAdminMap[grp_id] != params[2]) {
                res = "You are not the Admin\n";
                    cout<<"You are not the Admin"<<"\n";
            } else {
                grpIdMap[grp_id][user_name] = true;
                res = "User accepted to the group\n";
            }
            send(client_socket, res.c_str(), res.size()+1, 0);
        } else if (commandName == "list_requests") {
            if (params.size() != 2) {
                cout<<"InValid Params\n";
                send(client_socket, "NO", 2, 0);
            } else {
                    string user_name = params[1];
                    string grp_id = params[0];

                if (grpIdMap.find(grp_id) == grpIdMap.end())
                    {
                    string res = "InValid Group\n";
                    cout<<"InValid Group\n";
                    send(client_socket, res.c_str(), res.size()+1, 0);
                    }
                    
                else if (grpAdminMap[grp_id] != user_name) {
                    string res = "You dont have Access\n";
                    cout<<"You dont have Access\n";
                    send(client_socket, res.c_str(), res.size()+1, 0);
                }
                    else {
                    auto user_map = grpIdMap[params[0]];
                    string response = "List of Pending Requests\n";
                    for (auto it : user_map) {
                        if (it.second == false) {
                            string s = it.first+"\n";
                            response += s;
                        }
                    } 
                    response+="";
                    send(client_socket, response.c_str(), response.size()+1, 0);  
                }

            }
        } else if (commandName == "leave_group") {
            string grp_id = params[0];
            string user_name = params[1];

            if (grpIdMap.find(grp_id) == grpIdMap.end()) {
                string respone = "No such group found\n";
                send(client_socket, respone.c_str(), respone.size()+1, 0);
            } else if (grpIdMap[grp_id].find(user_name) == grpIdMap[grp_id].end()) {
                string respone = "User doesnt exist in the given group\n";
                send(client_socket, respone.c_str(), respone.size()+1, 0);
            } else {
                grpIdMap[grp_id].erase(user_name);
                if (grpAdminMap[grp_id] == user_name) {
                    for (auto user : grpIdMap[grp_id]) {
                        if (user.second == true) {
                            user_name = user.first;
                            break;
                        }
                    }
                    grpAdminMap[grp_id] = user_name;
                    if (user_name.empty()) {
                        grpIdMap.erase(grp_id);
                        grpAdminMap.erase(grp_id);
                    }
                }
                string response = "Successfully left the group\n";
                send(client_socket, response.c_str(), response.size()+1, 0);
            }
        }

        //Here upload and download redirections are done //
        else if (commandName == "upload_file") {
            UploadFile uploadFile;
            uploadFile.grpId = params[0];
            uploadFile.username = params[1];
            uploadFile.fileName = params[2];
            cout<<"Before the upload\n";
            cout<<"param[0] "<<params[0]<<"\n";
            cout<<"param[1] "<<params[1]<<"\n";
            cout<<"param[2] "<<params[2]<<"\n";
            cout<<"Before the upload\n";
            for (int i = 2; i< params.size(); i++) {
                uploadFile.hashes.emplace_back(params[i]);
            }
            updateUploads(uploadFile);
            string msg = "Uploaded the file successfully\n";
            send(client_socket, msg.c_str(), msg.size()+1, 0);
        } 
        
    
        else if (commandName == "download_file") {
                try {
                string name = params[3];
                string fileName = params[1];
                string grpId = params[0];


                trim(name);
                trim(grpId);
                trim(fileName);
                execute_download(name, fileName, grpId, client_socket);
                } catch (exception& e) {
                    string s = "Invalid params\n";
                    send(client_socket, s.c_str(), s.size()+1, 0);
                }
        }

        else {
            string res = "No such command found\n";
            send(client_socket, res.c_str(), res.size()+1, 0);
        }
    }
}


void Tracker::start_tracker() {
    cout << "Tracker is listening on port " << port << "...\n";
    
    struct sockaddr_in client_address; 
    socklen_t addrlen = sizeof(client_address); 
    
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
        if (new_socket < 0) {
            cerr << "Failed to accept connection\n";
        } else {
            cout << "Successful acceptance from client\n";
            cout << "client socket is : " << new_socket << "\n";
            thread client_thread(&Tracker::handleConnection, this, new_socket);
            client_thread.detach();
        }
    }
}

