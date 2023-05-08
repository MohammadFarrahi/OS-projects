#include "header.hpp"

using namespace std;


vector<string> split(string str, char delimiter) {
	string sub_str;
	stringstream stream(str);
	vector<string> result;

	while(getline(stream, sub_str, delimiter)){
		if(sub_str.size()) { result.push_back(sub_str); }
	}
	return result;
}

vector<string> get_file_names(string path) {
    string ls_cmd = "ls ", flags = " > ", temp_names_file = "names.txt";
    string command = ls_cmd + path + flags + temp_names_file;
    system(command.c_str());
    
    char buffer[BUFF_SIZE];
    int fd = open(temp_names_file.c_str(), O_RDONLY);
    if(fd < 0) { exit(1); }
    int read_bytes = read(fd, buffer, BUFF_SIZE);
    close(fd);
    unlink(temp_names_file.c_str());

    vector<string> names;
    size_t length = 0;
    for(int i=0; i<read_bytes; i++){
        if(buffer[i] != '\n') { length++; }
        else {
            names.push_back(path + string(buffer+i-length, length));
            length = 0;
        }
    }
    return names;
}
