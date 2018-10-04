#include "common.h"

void
write_to_file_append(std::string file, std::string msg) {
  std::ofstream ofile;

  ofile.open(file, std::ios::app);
  ofile << msg << std::endl;
  ofile.close();
}
