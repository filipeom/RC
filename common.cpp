#include "common.h"
#include "tcp.h"

std::string
read_string(int fd) {
  std::string c, str;

  c = read_msg(fd, 1);
  while(c.compare(" ")) {
    str.append(c);
    c = read_msg(fd, 1);
  }
  return str;
}

void
write_to_file_append(std::string file, std::string msg) {
  std::ofstream ofile;

  ofile.open(file, std::ios::app);
  ofile << msg; 
  ofile.close();
  return;
}

std::string
find_string(std::string key, std::string file) {
  std::string line;
  std::string msg;
  std::ifstream ifile;

  ifile.open(file);
  while(std::getline(ifile, line)) {
    std::size_t found = line.find(key);
    if(found != std::string::npos) {
      msg = line;
    }
  } 
  return msg;
}

std::string
get_files(std::string dirname) {
  int N = 0;
  DIR *dir;
  struct dirent *ent;
  struct stat stats;
  std::string files, msg;

  if((dir = opendir(dirname.c_str())) != NULL) {
    while((ent = readdir(dir)) != NULL) {
      std::string filename = ent->d_name;
      if(filename.compare(".") && filename.compare("..")) {
        char date_time[20];
        std::string path, file_stats, file_size;

        path = dirname + "/" + filename;
        stat(path.c_str(), &stats);
        strftime(date_time, 20, "%d.%m.%Y %H:%M:%S",
            localtime(&(stats.st_mtime)));
        file_size = std::to_string(stats.st_size);

        file_stats = filename + " " + date_time + " " + file_size + " ";
        files.append(file_stats);
        N++;
      }
    }
    msg = "BCK " + dirname + " " + std::to_string(N) + " ";
    msg.append(files);
    msg.append("\n");
    closedir(dir);
  } else {
    perror("opendir");
    exit(EXIT_FAILURE);
  }
  return msg;
}

std::string
find_user_and_check_pass(std::string file, std::string user,
    std::string pass) {
  std::string username, password, line, rply;
  std::ifstream ifile;

  ifile.open(file);
  while(std::getline(ifile, line)) {
    username = line.substr(0,5);
    password = line.substr(6,8);

    if(user.compare(username) == 0) {
      if(pass.compare(password) == 0) {
        std::cout << "User: " << user << std::endl;
        rply = "AUR OK\n";
      } else {
        rply = "AUR NOK\n";
      }
      return rply;
    }
  }
  ifile.close();
  write_to_file_append(file, user+" "+pass+"\n");
  std::cout << "New user: " << user << std::endl;
  rply = "AUR NEW\n";
  return rply;
}
