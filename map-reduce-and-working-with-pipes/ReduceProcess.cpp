#include "header.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    string process_result, maps_results = "";
    char buffer[BUFF_SIZE];
    int main_pipe_fd, result_file_fd, num_of_files;
    ostringstream buffer_stream;
    map<string, int> word_count;
    vector<string> key_value_words, key_value;

    // get args that passed to process
    main_pipe_fd = atoi(argv[1]);
    num_of_files = atoi(argv[2]);
    // getting content from fifo files
    for(int count = 0; count < num_of_files; count++){
        result_file_fd = open((MAP_RESULT_FILE + to_string(count)).c_str(), O_RDONLY);
        buffer[read(result_file_fd, buffer, BUFF_SIZE)] = '\0';
        close(result_file_fd);
        maps_results += string(buffer);
    }
    // reduce processing
    key_value_words = split(maps_results, NEW_LINE);
    for(auto& word : key_value_words) {
        key_value = split(word, COMMA);
        if(word_count.find(key_value[0]) == word_count.end()){ word_count[key_value[0]] = atoi(key_value[1].c_str()); }
        else { word_count[key_value[0]] += atoi(key_value[1].c_str()); }
    }
    // sending process result to main process
    for(auto const& word : word_count)
        buffer_stream << word.first << ':' << word.second << NEW_LINE;
    process_result = buffer_stream.str();
    write(main_pipe_fd, process_result.c_str(), process_result.size());
    // deleting fifo files
    for(int count = 0; count < num_of_files; count++){
        unlink((MAP_RESULT_FILE + to_string(count)).c_str());
    }
    close(main_pipe_fd);
    return 0;
}