#ifndef HEAD_HPP
#define HEAD_HPP "HEAD_HPP"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <map>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFF_SIZE 2048
#define FD_STR_LEN 4
#define NEW_LINE '\n'
#define COMMA ','
#define OUTPUT_FILE "output.csv"
const std::string TESTS_DIR_PATH = "./testcases/";  // if you want to change it, make sure it has slash at the end
const std::string MAP_PROCESS = "MapProcess";
const std::string REDUCE_PROCESS = "ReduceProcess";
const std::string MAP_RESULT_FILE = "map_result";

std::vector<std::string> split(std::string str, char delimiter);
std::vector<std::string> get_file_names(std::string path);

#endif