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
#define PORT 58043

int CSport = 0;

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
  rply = "AUR NEW\n";
  return rply;
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
  std::cout << ip << " " << port << std::endl;
  
  reply = "RGR OK\n";
  sendto(fd, reply.c_str(), reply.size(), 0,
      (struct sockaddr*)&addr,
      addrlen);

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

int
main(int argc, char **argv) {
  int pid, clientpid;
  std::string protocol;

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
      }
    }
    close(cs_udp_fd);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_FAILURE);
}
