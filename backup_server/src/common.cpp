#include <common.h>
#include <tcp.h>
#include <algorithm>

std::string
read_string(int fd) {
  std::string c, str;

  c = read_msg(fd, 1);
  while(c.compare(" ") && c.compare("\n")) {
    str.append(c);
    c = read_msg(fd, 1);
  }
  return str;
}

bool 
is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

void
change_file_time(std::string filename, std::string date,
    std::string time) {
  time_t mod_time;
  std::string date_time;
  struct utimbuf new_time;
  struct tm tm;

  date_time = date + " " + time + "\0";
  strptime(date_time.c_str(), "%d.%m.%Y %H:%M:%S", &tm);
  mod_time = mktime(&tm);
  
  new_time.modtime = mod_time;
  utime(filename.c_str(), &new_time);
  return;
}

void
remove_line_from_file_with_key(std::string key,
    std::string filename) {
  std::string line;
  std::ifstream ifile;
  std::ofstream ofile;

  ifile.open(filename);
  ofile.open("temp.txt");

  while(std::getline(ifile, line)) {
    std::size_t found = line.find(key);
    if(found == std::string::npos) {
      ofile << line << std::endl;
    }
  }
  ofile.close();
  ifile.close();
  remove(filename.c_str());
  rename("temp.txt", filename.c_str());
  return;
}

bool
check_if_string_exists_in_file(std::string key, std::string file) {
  std::string line;
  std::ifstream ifile;

  ifile.open(file);
  while(std::getline(ifile, line)) {
    std::size_t found = line.find(key);
    if(found != std::string::npos) {
      return true;
    }
  }
  return false;
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

        file_stats = " " + filename + " " + date_time + " " + file_size;
        files.append(file_stats);
        N++;
      }
    }
    msg = std::to_string(N);
    msg.append(files);
    msg.append("\n");
    closedir(dir);
  } else {
    perror("opendir");
    exit(EXIT_FAILURE);
  }
  return msg;
}

void
remove_all(std::string path) {
  DIR *dir;
  struct dirent *ent;

  if((dir = opendir(path.c_str())) != NULL) {
    while((ent = readdir(dir)) != NULL) {
      std::string filename = ent->d_name;
      if(filename.compare(".") && filename.compare("..")) {
        std::string full_path = path+"/"+filename;
        remove(full_path.c_str());
        std::cout << "Removing: " + path + "/" +filename + "...\n";
      }
    }
  }
  closedir(dir);
  remove(path.c_str());
  return;
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

bool
is_directory_empty(std::string dirname) {
  int n = 0;
  struct dirent *d;
  DIR *dir = opendir(dirname.c_str());
  while ((d = readdir(dir)) != NULL) {
    if(++n > 2)
      break;
  }
  closedir(dir);
  if (n <= 2) //Directory Empty
    return true;
  else
    return false;
}
