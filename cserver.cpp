#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unordered_map>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "tcp.h"
#include "udp.h"
#include "common.h"
#define PORT 58043

int CSport = 0;
std::string active_user;

/* CS TCP SERVER */
int cs_tcp_fd;
struct sockaddr_in cs_tcp_addr;

/* CS UDP SERVER */
int cs_udp_fd;
struct sockaddr_in cs_udp_addr;

/*BS UDP SERVER */
int bs_udp_fd;
struct sockaddr_in bs_udp_addr;

/*BS UDP CLIENT */
int bs_udp_client_fd;
struct sockaddr_in bs_udp_client_addr;
socklen_t bs_client_addr_len;

/* CLIENT TCP */
int client_fd;
struct sockaddr_in client_addr;
socklen_t client_len;

void
parse_input(int argc, char **argv) {
  int opt;
  extern int optind;

  while((opt = getopt(argc, argv, "p")) != -1) {
    switch(opt) {
      case 'p':
        CSport = atoi(argv[optind]);
        break;
      default:
        fprintf(stderr,"Usage: ./user [-p CSport]\n");
        exit(EXIT_FAILURE);
    }
  }

  if(CSport == 0) {
    CSport = PORT;
  }
  return;
}

std::string
find_user_and_check_pass(std::string user, std::string pass) {
  std::string username, password, line, rply; 
  std::ifstream ifile;
  std::ofstream ofile;

  ifile.open("cs_user_list.txt");
  while(std::getline(ifile, line)) {
    username = line.substr(0,5);
    password = line.substr(6,8);

    if(user.compare(username) == 0) {
      if(pass.compare(password) == 0) { 
        std::cout << "User: " << user << std::endl;
        active_user.clear(); active_user = user;
        rply = "AUR OK\n";
      } else {
        rply = "AUR NOK\n";
      }
      return rply;
    }
  }
  ifile.close();
  ofile.open("cs_user_list.txt", std::ios::app);
  ofile << user << " " << pass << std::endl;
  ofile.close();
  std::cout << "New user: " << user << std::endl;
  active_user.clear(); active_user = user;
  rply = "AUR NEW\n";
  return rply;
}

bool
check_if_bs_exists(std::string file, std::string ip,
    std::string port) {
  std::string line;
  std::ifstream ifile;

  ifile.open(file, std::ios::in);

  while(std::getline(ifile, line)) {
    std::string bs_ip, bs_port;
    int space;

    space = line.find(" ");
    bs_ip = line.substr(0, space);
    bs_port = line.substr(space + 1, line.size() - (space + 1));

    if((ip.compare(bs_ip) == 0) && (port.compare(bs_port) == 0)) {
      ifile.close();
      return true; 
    }
  }
  ifile.close();
  return false;
}

void
register_backup_server(int fd, struct sockaddr_in addr, 
    socklen_t addrlen, std::string str) {
  int space1;
  int space2;
  std::string ip, port, reply;

  space1 = str.find(" ");
  space2 = str.find(" ", space1+1);
  ip = str.substr(space1 + 1, space2 - (space1 + 1));
  port = str.substr(space2 + 1, (str.size()-1) - (space2+1));

  //TODO: RGR ERR
  if(check_if_bs_exists("bs_list.txt", ip, port)) {
    reply = "RGR NOK\n";
  } else {
    write_to_file_append("bs_list.txt", ip + " " + port);
    std::cout << "+BS: " + ip + " " + port << std::endl;
    reply = "RGR OK\n";
  }

  sendto(fd, reply.c_str(), reply.size(), 0,
      (struct sockaddr*)&addr,
      addrlen);

  return;
}


void
unregister_backup_server(int fd, struct sockaddr_in addr, 
    socklen_t addrlen, std::string str) {
  std::string ip, port, reply;
  std::string line, search_string;
  std::ifstream ifile;
  std::ofstream ofile;
  int space1, space2;

  space1 = str.find(" ");
  space2 = str.find(" ", space1+1);
  ip = str.substr(space1 + 1, space2 - (space1 + 1));
  port = str.substr(space2 + 1, (str.size()-1) - (space2+1));

  search_string = ip + " " + port;

  ifile.open("bs_list.txt");
  ofile.open("temp.txt");

  while(std::getline(ifile, line)) {
    if(line.compare(search_string) != 0)
      ofile << line << std::endl;
    else
      reply = "UAR OK\n";
  }
  ofile.close();
  ifile.close();
  remove("bs_list.txt");
  rename("temp.txt", "bs_list.txt");

  sendto(fd, reply.c_str(), reply.size(), 0,
      (struct sockaddr*) &addr, addrlen);

  //TODO UAR NOK/ERR
  return; 
  //should never happen
  reply = "UAR NOK\n";
  return;
}

void
auth_user() {
  std::string user, pass;
  std::string response;

  read_msg(client_fd, 1);
  user = read_msg(client_fd, 5);
  read_msg(client_fd, 1);
  pass = read_msg(client_fd, 8);
  read_msg(client_fd, 1);

  response = find_user_and_check_pass(user, pass);
  write_msg(client_fd, response);
  return;
}


void
delete_user() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  //TODO: ALL
  return;
}

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

std::string
read_file_list(std::string &dir, int &N) {
  int i;
  std::string file_list;

  dir = read_string(client_fd);
  N = stoi(read_string(client_fd));

  for(i = 0; i < N; i++) {
    std::string line;
    std::string filename, date, time, size;

    filename = read_string(client_fd);
    date = read_string(client_fd);
    time = read_string(client_fd);
    size = read_string(client_fd);

    line = filename+" "+date+" "+time+" "+size+" ";
    file_list.append(line);
  }
  //READ TRAILING \n
  read_msg(client_fd, 1);
  file_list.append("\n");
  return file_list;
}

bool
find_user_dir(std::string dir, std::string user, std::string &ip,
    std::string &port) {
  std::string line;
  std::ifstream file;

  file.open("backup_list.txt");
  while(std::getline(file, line)) {
    std::size_t found = line.find(user);
    if(found != std::string::npos) {
      found = line.find(dir);
      if(found != std::string::npos) {
        int space1, space2, space3;

        space1 = line.find(" ");
        space2 = line.find(" ", space1 + 1);
        space3 = line.find(" ", space2 + 1);

        ip = line.substr(space1 + 1, (space2 - space1)-1);
        port = line.substr(space2 + 1, (space3 - space2)-1);
        file.close();
        return true;
      }
    }
  }
  return false;
}

bool
find_user_bs(std::string user, std::string &ip, std::string &port) {
  std::string line;
  std::ifstream file;

  file.open("backup_list.txt");
  while(std::getline(file, line)) {
    std::size_t found = line.find(user);
    if(found != std::string::npos) {
      int space1, space2, space3;

      space1 = line.find(" ");
      space2 = line.find(" ", space1 + 1);
      space3 = line.find(" ", space2 + 1);

      ip = line.substr(space1 + 1, (space2 - space1)-1);
      port = line.substr(space2 + 1, (space3 - space2)-1);
      file.close();
      return true;
    }
  }
  return false;
}

void
get_bs_for_user(std::string &ip, std::string &port) {
  int space;
  std::string line;
  std::ifstream file;

  file.open("bs_list.txt");
  std::getline(file, line);
  
  space = line.find(" ");
  ip = line.substr(0, space);
  port = line.substr(space+1, line.size() - (space+1));
  return;
}

void
backup_user_dir() {
  int N;
  std::string dirname, file_list;
  std::string bs_ip, bs_port;
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);

  file_list = read_file_list(dirname, N);
  std::cout << "CS: " << dirname << " " << N << " ";
  std::cout << file_list;
  if(find_user_dir(dirname, active_user, bs_ip, bs_port)) {
    //ASK BS FOR FILES
    std::cout << bs_ip << " " << bs_port << std::endl;
  } else if(find_user_bs(active_user, bs_ip, bs_port)) {
    //IF USER IS REGISTERED IN A BS SEND FILES THERE
    std::cout << bs_ip << " " << bs_port << std::endl;
  } else {
    get_bs_for_user(bs_ip, bs_port);
    std::cout << bs_ip << " " << bs_port << std::endl;
  }
  return;
}

void
restore_user_dir() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  //TODO: ALL
  return;
}

void
dir_list() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  //TODO: ALL
  return;
}

void
file_list() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  //TODO: ALL
  return;
}

void
delete_dir() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  //TODO: ALL
  return;
}

int
main(int argc, char **argv) {
  int pid, clientpid;
  std::string protocol;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  parse_input(argc, argv);

  if((pid = fork()) == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
    /* CHILD HANDLES INCOMING CLIENTS*/
  } else if (pid == 0) {
    create_central_server_tcp(cs_tcp_fd, cs_tcp_addr, CSport);
    while(true) {    
      client_len = sizeof(client_addr);
      client_fd = accept(cs_tcp_fd, (struct sockaddr*)&client_addr,
          &client_len);

      if((clientpid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
      } else if(clientpid == 0) {
        close(cs_tcp_fd);

        protocol = read_msg(client_fd, 3);
        if(protocol.compare("AUT") == 0) {
          auth_user();

          protocol.clear(); protocol = read_msg(client_fd, 3);
          if(protocol.compare("DLU") == 0) {
            delete_user();
          } else if(protocol.compare("BCK") == 0) {
            backup_user_dir();
          } else if(protocol.compare("RST") == 0) {
            restore_user_dir();
          } else if(protocol.compare("LSD") == 0) {
            dir_list();
          } else if(protocol.compare("LSF") == 0) {
            file_list();
          } else if(protocol.compare("DEL") == 0) {
            delete_dir();
          }
        }

        close(client_fd);
        exit(EXIT_SUCCESS);
      }
    }
    close(cs_tcp_fd);
    exit(EXIT_FAILURE);
    /* PARENT */
  } else {
    create_central_server_udp(cs_udp_fd, cs_udp_addr, CSport);
    while(true) {
      char buffer[128] = {0};

      bs_client_addr_len = sizeof(bs_udp_client_addr);
      recvfrom(cs_udp_fd, buffer, sizeof(buffer), 0,
          (struct sockaddr*)&bs_udp_client_addr,
          &bs_client_addr_len);

      if(strncmp(buffer, "REG", 3) == 0) {
        register_backup_server(cs_udp_fd, bs_udp_client_addr,
            bs_client_addr_len, buffer);
      }else if(strncmp(buffer, "UNR", 3) == 0) {        
        unregister_backup_server(cs_udp_fd, bs_udp_client_addr,
            bs_client_addr_len, buffer);
      }
    }
    close(cs_udp_fd);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_FAILURE);
}
