#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "tracker.h"

using namespace std;

track_info parseTrackerInfo(const vector<string>& params) {
    string file_path = params[0];
    int line_number = stoi(params[1]);

    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        cerr << "Error: Unable to open file " << file_path << endl;
        return {"", -1};
    }

    char buffer[1024];
    ssize_t bytes_read;
    int current_line = 0;
    string current_content;
    track_info result = {"", -1};

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        current_content.append(buffer, bytes_read);
        size_t pos;
        while ((pos = current_content.find('\n')) != string::npos) {
            string line = current_content.substr(0, pos);
            current_line++;
            
            if (current_line == line_number) {
                size_t colon_pos = line.find(':');
                if (colon_pos != string::npos) {
                    result.ip = line.substr(0, colon_pos);
                    result.port = stoi(line.substr(colon_pos + 1));
                } else {
                    cerr << "Error: Invalid format in line " << line_number << endl;
                }
                close(fd);
                return result;
            }
            
            current_content = current_content.substr(pos + 1);
        }
    }

    close(fd);

    if (current_line < line_number) {
        cerr << "Error: Line " << line_number << " not found in the file" << endl;
    }

    return result;
}

int main(int argc, char* argv[]) {
    vector<string> params;
    if (argc != 3) {
        cout << "Invalid arguments\n";
        return 0;
    }
    params.push_back(argv[1]);
    params.push_back(argv[2]);
    track_info tf;
    tf = parseTrackerInfo(params);
    Tracker tracker(tf);
    tracker.start_tracker();

}