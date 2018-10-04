#include "common.h"

void
write_to_file_append(std::string file, std::string msg) {
  std::ofstream ofile;

  ofile.open(file, std::ios::app);
  ofile << msg << std::endl;
  ofile.close();
  return;
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
