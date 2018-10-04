#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "tcp.h"
#include "common.h"
#define PORT 58043

int CSport = 0;
bool logged = false;
std::string CSname, auth_str;

int cs_tcp_fd;
struct hostent *cs_host;
struct sockaddr_in cs_tcp_addr;

void
parse_input(int argc, char **argv) {
  int opt;
  extern int optind;

  while ((opt = getopt(argc, argv, "np")) != -1) {
    switch(opt) {
      case 'p':
        CSport = atoi(argv[optind]);
        break;
      case 'n':
        CSname = argv[optind];
        break;
      default:
        fprintf(stderr,"Usage: ./user [-n CSname] [-p CSport]\n");
        exit(EXIT_FAILURE);
    }
  }

  if(CSport == 0) {
    CSport = PORT;
  }
  if(CSname.empty()) {
    char buffer[128];

    gethostname(buffer, 128);
    CSname.assign(buffer, strlen(buffer));
  }
  return;
}

void
login() {
  std::string user, pass;
  std::string auth_reply;

  if(logged != true) {
    std::cin >> user >> pass;
    connect_to_central_server(cs_tcp_fd, cs_tcp_addr, cs_host,
        CSname, CSport);

    auth_str = "AUT " + user + " " + pass + "\n";
    write_msg(cs_tcp_fd, auth_str);
    auth_reply = read_msg(cs_tcp_fd, 3);

    if(auth_reply.compare("AUR") == 0){
      read_msg(cs_tcp_fd, 1);
      auth_reply.clear(); auth_reply = read_msg(cs_tcp_fd, 3);

      if(auth_reply.compare("NEW") == 0) {
        std::cout << "User \"" + user + "\" created" << std::endl;
        logged = true;
        read_msg(cs_tcp_fd, 1);
      } else if(auth_reply.compare("NOK") == 0) {
        std::cout << "[!] Auth was unsuccessful." << std::endl;
        read_msg(cs_tcp_fd, 1);
        auth_reply.clear();
        logged = false;
      } else {
        std::cout << "OK!" << std::endl;
        logged = true;
      }
    }
    close(cs_tcp_fd);
  } else {
    std::cout << "[!] User: \"" + auth_str.substr(4, 5) + "\" is logged in.\n";
  }
  return;
}

void
deluser() {
  return;
}

void
backup() {
  std::string auth_reply, dirname, file_list;
  std::cin >> dirname;

  if(logged) {
    connect_to_central_server(cs_tcp_fd, cs_tcp_addr, cs_host,
        CSname, CSport);
    
    write_msg(cs_tcp_fd, auth_str);
    auth_reply = read_msg(cs_tcp_fd, 3);

    if(auth_reply.compare("AUR") == 0) {
      read_msg(cs_tcp_fd, 1); auth_reply.clear();
     
      auth_reply = read_msg(cs_tcp_fd, 3);
      if(auth_reply.compare("OK\n") == 0) {
        /* THIS STRING ENDS WITH " \n"(space and newline chars) */
        file_list = get_files(dirname);
        write_msg(cs_tcp_fd, file_list);
      } else {
        read_msg(cs_tcp_fd, 1);
        std::cout << "[!] Something wrong: AUT NOK\n";
      }

    }
    close(cs_tcp_fd);
  } else {
    std::cout << "[!] No available session to backup from.\n";
  }
  return;
}

void
restore() {
  return;
}

void
dirlist() {
  return;
}

void
filelist() {
  return;
}

void
delete_user() {
  return;
}

void
logout() {
  logged = false;
  auth_str.clear();
}

int
main(int argc, char **argv) {
  parse_input(argc, argv);

  while(true) {
    std::string input;
    std::cin >> input;

    if(input.compare("login") == 0) {
      login();
    } else if(input.compare("deluser") == 0) {
      deluser();
    } else if(input.compare("backup") == 0) {
      backup();
    } else if(input.compare("restore") == 0) {
      restore();
    } else if(input.compare("dirlist") == 0) {
      dirlist();
    } else if(input.compare("filelist") == 0) {
      filelist();
    } else if(input.compare("delete") == 0) {
      delete_user();
    } else if(input.compare("logout") == 0) {
      logout();
    } else if(input.compare("exit") == 0) {
      exit(EXIT_SUCCESS);
    }
  }
  exit(EXIT_FAILURE);
}
