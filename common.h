#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <utime.h>
#include <sys/types.h>

bool is_directory_empty(std::string dirname);
std::string read_string(int fd);
void change_file_time(std::string, std::string, std::string);
void write_to_file_append(std::string file, std::string msg);
void remove_all(std::string path);
bool check_if_string_exists_in_file(std::string key, std::string file);
void remove_line_from_file_with_key(std::string key, std::string file);
std::string find_string(std::string key, std::string file);
std::string get_files(std::string dirname);
std::string find_user_and_check_pass(std::string file,
    std::string user, std::string pass);
bool is_number(const std::string& s);
#endif
