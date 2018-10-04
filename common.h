#include <iostream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>

std::string read_string(int fd);
void write_to_file_append(std::string file, std::string msg);
std::string find_string(std::string key, std::string file);
std::string get_files(std::string dirname);
std::string find_user_and_check_pass(std::string file,
    std::string user, std::string pass);
