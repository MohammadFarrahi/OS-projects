#include "header.hpp"

using namespace std;


int main(int argc, char* argv[]) {
    string process_result;
    char buffer[BUFF_SIZE];
    int main_pipe_fd, file_fd;
    ostringstream buffer_stream;
    map<string, int> word_count;
    vector<string> lines_of_file, words;

    // making named pipe(fifo file)
    int postfix = -1;
    while(mkfifo((MAP_RESULT_FILE + to_string(++postfix)).c_str(), S_IRWXG | S_IRWXU) < 0);
    //getting file name via pipe
    main_pipe_fd = atoi(argv[1]);
    buffer[read(main_pipe_fd, buffer, BUFF_SIZE)] = '\0';
    close(main_pipe_fd);
    //getting file content
    file_fd = open(string(buffer).c_str(), O_RDONLY);
    if(file_fd < 0) { exit(1); }
    memset(buffer, 0, string(buffer).size());
    buffer[read(file_fd, buffer, BUFF_SIZE)] = '\0';
    close(file_fd);
    // processing on content
    lines_of_file = split(string(buffer), NEW_LINE);
    for(auto& line : lines_of_file){
        words = split(line, COMMA);
        for(auto& word : words){
            if(word_count.find(word) == word_count.end()){ word_count[word] = 1; }
            else { word_count[word]++; }
        }
    }
    //saving reuslt on fifo
    for(auto const& word : word_count)
        buffer_stream << word.first << COMMA << word.second << NEW_LINE;
    process_result = buffer_stream.str();
    file_fd = open((MAP_RESULT_FILE + to_string(postfix)).c_str(), O_WRONLY);
    write(file_fd, process_result.c_str(), process_result.size());
    close(file_fd);
    return 0;
}