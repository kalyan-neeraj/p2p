#include "user.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
#include <thread>
#include <algorithm>

using namespace std;

class Client
{
struct sockaddr_in tracker_address;

//Chunk struct since we are going to use it again and again //
struct ChunkData {
    string number;
    string hash;
};

struct ResponseData {
    int number;
    string hash;
    vector<string>users;
};


struct UploadFile {
    string grpId;
    string username;
    vector<string>hashes;
    string fileName;
};


private:
    string clientName;
    string clientPassWord;
    int client_fd;
    int port;
    bool isLoggedIn;
    struct sockaddr_in client_address;
    unordered_map<string, vector<ChunkData>> chunk_store;
    unordered_map<string, string>file_name_pathMap;
    string tracker_info_path;
public:
Client(vector<string>params) {
    struct Input input;
    getInput(input, params);
    this->port = input.port;
    this->client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->client_fd < 0) {
        cout << "Socket creation failed\n";
        exit(EXIT_FAILURE);
    }

    //Set up the client address structure//
    this->client_address.sin_family = AF_INET;
    cout<<input.ip<<"\n";
    
    if (inet_pton(AF_INET, input.ip.c_str(), &this->client_address.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported\n";
        exit(EXIT_FAILURE);
    }
    this->client_address.sin_port = htons(port);
    cout<<port<<"\n";

    // Attempt to bind the socket to the specified IP and port
    if (bind(this->client_fd, (struct sockaddr *)&this->client_address, sizeof(this->client_address)) < 0) {
        if (errno == EADDRINUSE) {
            cout << "Port already in use. Try a different port.\n";
        } else {
            cout << "Bind failed: " << strerror(errno) << endl;
        }
        exit(EXIT_FAILURE);
    }

    this->isLoggedIn = false;
    this->clientName = "";
    this->clientPassWord = "";
    this->tracker_info_path = input.filepath;
}



void handle_peer_request(int socket) {
    char buffer[1024] = {0};
    
    // Read the message sent by the peer
    int valread = read(socket, buffer, 1024);
    if (valread < 0) {
        perror("read failed");
    } else {
        cout << "Received request: " << buffer << "\n";
        
        // Here, you can parse the request and respond accordingly
        string response = "Request processed";
        send(socket, response.c_str(), response.size(), 0);
    }

    // Close the socket after processing the request
    close(socket);
}



void listen_for_requests() {
    cout << "Client is listening on port " << port << "...\n";
    
    struct sockaddr_in client_address; 
    socklen_t addrlen = sizeof(client_address); 
    
    while (int new_socket = accept(this->client_fd, (struct sockaddr *)&client_address, &addrlen) > 0) {
        // Accept new connections from peers
        if (new_socket < 0) {
            cerr << "Failed to accept connection\n";
        } else {
            cout << "Connection accepted from peer\n";
            cout << "Peer socket is: " << new_socket << "\n";
            
            // Handle each peer connection in a separate thread
            thread client_thread(&Client::handle_peer_request, this, new_socket);
            client_thread.detach();
        }
    }
}


    ~Client() {
        close(this->client_fd);
    }



// Idea is to store in hex rather than binary since it takes up half the space as a key//
char to_hex_char(char c) {
    if (c < 10) return '0' + c;
    else return 'a' + (c - 10);
}

void encrypt(const char* data, unsigned long long length, char* chunk_hash) {
    //Initialize SHA-256 context
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    //Feed the data (chunk) into the hash function
    SHA256_Update(&sha256, data, length);

    SHA256_Final(hash, &sha256);

    //Convert the hash to a hex string manually
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        //High nibble (4 bits)
        chunk_hash[i * 2]     = to_hex_char((hash[i] >> 4) & 0xF);
         //Low nibble (4 bits)
        chunk_hash[i * 2 + 1] = to_hex_char(hash[i] & 0xF);
    }
    chunk_hash[SHA256_DIGEST_LENGTH * 2] = '\0';
}



string uploadFileByClient(string& filePath) {
    cout<<"\n"<<"File path we got : "<<filePath.size()<<"\n";
        trim(filePath);
        int fd = open(filePath.c_str(), O_RDONLY);
        if (fd < 0) {
            cout << "Failed to open file\n";
            return "";
        }

        string filename = getFileName(filePath);

        //In future to download chunks from file at required offsets//
        file_name_pathMap[filename] = filePath;

        struct UploadFile uploadFile;
        uploadFile.username = this->clientName;
        uploadFile.fileName = filename;

        long long buffer_size = 1024* 512;
        long long bytes_read = 0;
        char* buffer = new char[buffer_size];
        char chunk_hash[SHA256_DIGEST_LENGTH * 2 + 1];

        string chunks;
        int chunk_number = 0;
        vector<string>chunk_st;
        vector<ChunkData>chunkStore;

while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        encrypt(buffer, bytes_read, chunk_hash);
                string hashed_chunk(chunk_hash);
                ChunkData chunk;
                chunk.hash = hashed_chunk;
                chunks = chunks + to_string(chunk_number) +":" +chunk.hash;
                chunk_st.emplace_back(chunk.hash + " ");
                chunk_number++;
                chunkStore.emplace_back(chunk);
    }
        string token;
        chunk_store[filename] = chunkStore;
        token += uploadFile.username + " " + uploadFile.fileName + " " + uploadFile.grpId;
        for (auto& chunk : chunk_st) {
            token = token + " " + chunk + " ";
        }
        cout<<chunk_number<<"\n";
        return token;
    }

void connectToTracker() {
    vector<tracker_info> tracker_inf = getTrackerInfo(this->tracker_info_path);
    bool connected = false;

    for (const auto& tracker : tracker_inf) {
        tracker_address.sin_family = AF_INET;
        tracker_address.sin_port = htons(tracker.port);

        if (inet_pton(AF_INET, tracker.ip.c_str(), &tracker_address.sin_addr) <= 0) {
            cerr << "Invalid tracker IP address: " << tracker.ip << "\n";
            continue; 
        }

        if (connect(this->client_fd, (struct sockaddr*)&tracker_address, sizeof(tracker_address)) < 0) {
            cout << "Failed to connect to tracker: " << tracker.ip << ":" << tracker.port << "\n";
        } else {
            cout << "Connected to tracker " << tracker.ip << ":" << tracker.port << " successfully!\n";
            connected = true;
            break;
        }
    }

    if (!connected) {
        cout<<"Trackers are offline\n";
        return;
    }
}

string trim(string& str) {
    if (str.empty()) return "";

    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\0' || std::isspace(str[start]))) {
        ++start;
    }

    if (start == str.length()) {
        str.clear();
        return "";
    }

    size_t end = str.length() - 1;
    while (end > start && (str[end] == ' ' || str[end] == '\0' || std::isspace(str[end]))) {
        --end;
    }
   return str = str.substr(start, end - start + 1);
}


void parse_response(string & response, string& command, string& value) {
        int pos = response.find_last_of(':');
            if (pos > 0) {
                command = response.substr(0, pos);
                value = response.substr(pos+1);
        }
    }

void handle_response(string& command, string &val) {
if (command == "create_user") {
    long long firstSpace = val.find(' ');
    if (firstSpace != string::npos) {
        this->clientName = val.substr(0, firstSpace);

        long long secondSpace = val.find(' ', firstSpace + 1);
        if (secondSpace == string::npos) {
            this->clientPassWord = val.substr(firstSpace + 1);
        } else {
            this->clientPassWord = val.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        }

        trim(this->clientName);
        trim(this->clientPassWord);

        if (this->clientName.empty() || this->clientPassWord.empty()) {
            this->clientName.clear();
            this->clientPassWord.clear();
        }
    } else {
        this->clientName.clear();
        this->clientPassWord.clear();
    }
    cout<<"User "<<this->clientName<<"created Successfully"<<"\n";
    } else {
        cout<<command<<" "<<val<<endl;
    }
};


void grpFnp(string& grpId, string& filename, string & token, int i) {
    int j = i+1;
        while (j < token.size() && token[j] != ' ') {
            j++;
        }

     while (j < token.size() && token[j] != ' ') {
            j++;
        }

        grpId = token.substr(i+1, j - i-1);

        if (j < token.size()) {
            filename = token.substr(j);
        }

        // Trim leading and trailing whitespace from file_path
        trim(filename);
        trim(grpId);

}



string getWholeMessage(int client_socket) {
    vector<char> token;
    const unsigned long long buffer_size = 4096;
    char buffer[buffer_size];
    cout << "Started....Reading from Client\n";
    long long valrecv;
    while ((valrecv = recv(client_socket, buffer, buffer_size, 0)) > 0) {
        buffer[valrecv] = '\0';
        token.insert(token.end(), buffer, buffer + valrecv);
        // Check for the end of message (you might want to implement a proper protocol)
        if (strchr(buffer, '\0') != nullptr) {
            break;
        }
    }

    if (valrecv == 0) {
        cout << "Client disconnected\n";
    } else if (valrecv < 0) {
        cerr << "Failed to recv from socket: " << strerror(errno) << endl;
    }

    cout << "Ended....Reading from Client\n";
    return string(token.begin(), token.end());
}


/****************************************************Download Section*******************************************************************/

// Function to establish a socket connection with a user
bool connect_to_user(const string& user) {
    string user_ip = "";
    string port;

    // Extract IP and port from the user string
    int n = user.size();
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (user[i] == ':') {
            j = i;
            break;
        }
    }

    if (j == 0) return false;

    user_ip = user.substr(0, j);
    port = user.substr(j + 1);
    trim(user_ip);
    trim(port);

    // Create the socket
   int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return false;
    }

    // Convert port from string to integer
    int prt;
    try {
        prt = stoi(port);
        cout<<prt<<"\n";
    } catch (const invalid_argument& e) {
        cerr << "Invalid port: " << port << "\n";
        close(sock);
        return false;
    }

    // Prepare server address
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(prt);

    // Convert IP from string to binary form
    if (inet_pton(AF_INET, user_ip.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Invalid IP address: " << user_ip << "\n";
        close(sock);
        return false;
    }

    // Attempt to connect to the user
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return false;
    }

    cout << "Connection to " << user_ip << " established on port " << prt << "\n";
    close(sock);
    return true;
}

void handle_tracker_meta(string& response, vector<ResponseData>& torrent) {
     size_t pos = 0;
    string line;
    while ((pos = response.find('\n')) != string::npos) {
        line = response.substr(0, pos);
        response.erase(0, pos + 1);
        
        ResponseData data;
        
        // Find the colon that separates hash_no from users
        size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            // Parse hash_no
            size_t underscore_pos = line.find('_');
            if (underscore_pos != string::npos && underscore_pos < colon_pos) {
                data.hash = line.substr(0, underscore_pos);
                data.number = stoi(line.substr(underscore_pos + 1, colon_pos - underscore_pos - 1));
            }
            
            // Parse users
            string users_str = line.substr(colon_pos + 1);
            size_t user_pos = 0;
            string user;
            while ((user_pos = users_str.find(' ')) != string::npos) {
                user = users_str.substr(0, user_pos);
                data.users.push_back(user);
                users_str.erase(0, user_pos + 1);
            }
            if (!users_str.empty()) {
                data.users.push_back(users_str);
            }
            
            torrent.push_back(data);
        }
    }
}


void download_chunk(ResponseData& chunk_data) {
        for (const auto& user : chunk_data.users) {
            if (connect_to_user(user)) {
                // Connection successful, proceed with downloading the chunk
                cout << "Downloading chunk " << chunk_data.number << " from " << user << "\n";
                // Add chunk download logic here after connection is established.
                break;
            }
        }
    }

void handle_download_threads(vector<ResponseData>& torrent) {
    // Sort based on the rarity of chunks
    sort(torrent.begin(), torrent.end(), [](const ResponseData& a, const ResponseData& b) {
        return a.users.size() < b.users.size(); 
    });
    
    vector<thread> download_threads;
    
    // Loop through the torrent, create a thread for each chunk
    for (size_t i = 0; i < torrent.size(); ++i) {
        download_threads.emplace_back(&Client::download_chunk, this, ref(torrent[i]));
    }
    
    // Join all threads after they finish
    for (auto& t : download_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    download_threads.clear();
}


void handle_download(string& response, string& download_path) {
    cout<<response<<"\n";
    trim(response);
    vector<ResponseData>torrent;
    handle_tracker_meta(response, torrent);
    // if (response.size()) torrent.pop_back();
    cout<<"Size: "<<torrent.size()<<"\n";
        if (torrent.empty() || torrent.size() == 0) {
        cout << "No chunks to download." << "\n";
        return;
    }
    handle_download_threads(torrent);
}

void getDownloadPath(string& token, string& path) {
    int n = token.size();
    int j = 0;
    for (int i = n-1; i>=0; i--) {
        if (token[i] == ' ') {
            j = i;
            break;
        }
    }
    if (j == 0) {
        cout<<"Not a Valid Path\n";
    }
    path = token.substr(j+1);
}


/*******************************Download Section*******************************************************************/


void execute_command(string& token) {
    string cmd = "";
    int i = 0;
    while (i < token.size()) {
        if (token[i] == ' ') {
            cmd = token.substr(0, i);
            break;
        }
        i++;
    }
    string new_string = token;
    string download_path = "";
     if ((cmd == "create_group" || cmd == "join_group") || (cmd == "leave_group" || cmd == "list_requests") || (cmd == "accept_request" || cmd == "list_groups") || (cmd == "download_file")) {
            if (cmd == "download_file") {
                getDownloadPath(new_string, download_path);
            }
            new_string +=  " " + this->clientName;
    } else if (cmd == "upload_file") {
        int j = i+1;
        string grpId = "";
        string file_path = "";
        while (j < token.size() && token[j] != ' ') {
            j++;
        }

        file_path = token.substr(i+1, j - i-1);

        if (j < token.size()) {
            grpId = token.substr(j);
        }

        // Trim leading and trailing whitespace from file_path
        trim(file_path);
        trim(grpId);
        string meta_data = uploadFileByClient(file_path);
        if (meta_data.empty()) {
            return;
        }
        string cmd = "upload_file " + grpId +" " + meta_data;
        new_string = cmd;
    } else if (cmd == "create_user") {
        new_string +=  " " + string(inet_ntoa(client_address.sin_addr)) + ":" + to_string(ntohs(client_address.sin_port));
    }


    if (send(this->client_fd, new_string.c_str(), new_string.size()+1, 0) < 0) {
            cerr << "Failed to send command to tracker: " << strerror(errno) << endl;
        } else {
            cout << "Command sent to tracker: " << new_string << endl;
        }

        //Trceker respsonse//
        string response = getWholeMessage(this->client_fd);
        if (response.empty()) {
            cerr << "No response received from server" << endl;
            return;
        }

        if (cmd == "download_file") {
            cout<<"Starting download\n";
            handle_download(response, download_path);
        } else {
            string command = "";
        string val = "";
        parse_response(response, command, val);
        if (!command.empty()) {
            handle_response(command, val);
        } else {
            cout << "Received response: " << response << endl;
        }
        }
    }
};
