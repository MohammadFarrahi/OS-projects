#include "header.hpp"

using namespace std;


int main(){
    char buffer[BUFF_SIZE];
    char pipe_fd_str[FD_STR_LEN], num_of_files_str[FD_STR_LEN];
    int map_pipe[2], reduce_pipe[2], pid, status;
    vector<string> file_names;
    
    // get name of working files
    file_names = get_file_names(TESTS_DIR_PATH);
    // creating map processes
    if (pipe(map_pipe) < 0) { exit(1); }
    snprintf(pipe_fd_str, FD_STR_LEN, "%d", map_pipe[0]);
    char* argv[] = { (char*)MAP_PROCESS.c_str(), pipe_fd_str, NULL, NULL };     // making argv for map processes
    for(auto const& file_name : file_names){
        if((pid = fork()) < 0){ exit(1); }
        else if(pid == 0) {
            close(map_pipe[1]);
            execv(MAP_PROCESS.c_str(), (char* const*)argv);
        }
        else {
            write(map_pipe[1], file_name.c_str(), file_name.size());
            sleep(1);
        }
    }
    // creating reduce process
    if (pipe(reduce_pipe) < 0) { exit(1); }
    argv[0] = (char*)REDUCE_PROCESS.c_str();
    snprintf(pipe_fd_str, FD_STR_LEN, "%d", reduce_pipe[1]);
    snprintf(num_of_files_str, FD_STR_LEN, "%d", (int)file_names.size());
    argv[2] = num_of_files_str;
    if((pid = fork()) < 0){ exit(1); }
    else if(pid == 0) {
        close(reduce_pipe[0]);
        execv(REDUCE_PROCESS.c_str(), (char* const*)argv);
    }
    else {
        int content_size = read(reduce_pipe[0], buffer, BUFF_SIZE);
        buffer[content_size] = '\0';
        unlink(OUTPUT_FILE); int output_file_fd;
        if((output_file_fd = open(OUTPUT_FILE, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXU)) < 0){ exit(0); };
        write(output_file_fd, buffer, content_size);
        close(output_file_fd);
    }
    while (wait(&status) > 0);
    close(reduce_pipe[0]);
    close(reduce_pipe[1]);
    close(map_pipe[0]);
    close(map_pipe[1]);
    return 0;
}
