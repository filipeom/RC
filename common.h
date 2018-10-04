#include <iostream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

void write_to_file_append(std::string file, std::string msg);
std::string get_files(std::string dirname);
