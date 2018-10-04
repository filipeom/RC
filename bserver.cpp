#include <iostream>
#include <csignal>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unordered_map>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "tcp.h"
#include "udp.h"
#define PORT_CS 58043
#define PORT_BS 59043

int CSport = 0, BSport = 0;
std::string CSname;

/* BS UDP SERVER */
int bs_udp_fd;
struct sockaddr_in bs_udp_addr;

/* BS TCP SERVER */
int bs_tcp_fd;
struct sockaddr_in bs_tcp_addr;

/* CS UDP SERVER */
int cs_udp_fd;
struct sockaddr_in cs_udp_addr;
struct hostent *cs_host;
int cs_addrlen;

/* CS UDP CLIENT */
int cs_udp_client_fd;
struct sockaddr_in cs_udp_client_addr;
socklen_t cs_client_addr_len;

/* CLIENT TCP */
int client_fd;
struct sockaddr_in client_addr;
socklen_t client_len;

void
parse_input(int argc, char **argv) {
  int opt;
  extern int optind;

  while((opt = getopt(argc, argv, "npb")) != -1) {
    switch(opt) {
      case 'p':
        CSport = atoi(argv[optind]);
        break;
      case 'b':
        BSport = atoi(argv[optind]);
        break;
      case 'n':
        CSname = argv[optind];
        break;
      default:
        fprintf(stderr, "Usage: ./BS [-p CSport] [-n CSname] [-b BSname]\n");
        exit(EXIT_FAILURE);
    }
  }

  if(CSport == 0) CSport = PORT_CS;
  if(BSport == 0) BSport = PORT_BS;
  if(CSname.empty()) {
    char buffer[128] = {0};

    gethostname(buffer, 128);
    CSname.assign(buffer, strlen(buffer));
  }
}

std::string
get_bs_ip() {
  char buffer[128] = {0};
  struct hostent *h;
  struct in_addr *a;

  if(gethostname(buffer, 128) == -1) {
    perror("gethostname");
    exit(EXIT_FAILURE);
  }
  if((h = gethostbyname(buffer)) == NULL) {
    fprintf(stderr, "gethostbyname: no host.\n");
    exit(EXIT_FAILURE);
  }
  a = (struct in_addr*) h->h_addr_list[0];
  return inet_ntoa(*a); 
}

void
register_backup_server(int fd, struct sockaddr_in addr, int addrlen) {
  std::string reg;
  char buffer[128] = {0};

  reg = "REG " + get_bs_ip() + " " + std::to_string(BSport) + "\n";
  sendto(fd, reg.c_str(), reg.size(), 0,
      (struct sockaddr*)&addr,
      addrlen);

  recvfrom(fd, buffer, sizeof(buffer), 0,
      (struct sockaddr*)&addr,
      (socklen_t*)&addrlen);
  std::cout << buffer;
  return;
}

int
main(int argc, char **argv) {
  int pid, clientpid;
  
  parse_input(argc, argv);

  if((pid = fork()) == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
    /* CHILD HANDLES INCOMING CLIENTS */
  } else if (pid == 0) {
    create_backup_server_tcp(bs_tcp_fd, bs_tcp_addr, BSport);
    while(true) {
      client_len = sizeof(client_addr);
      client_fd = accept(bs_tcp_fd, (struct sockaddr*)&client_addr,
          &client_len);

      if((clientpid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
      } else if(clientpid == 0) {
        close(bs_tcp_fd);
        close(client_fd);
        exit(EXIT_SUCCESS);
      }
    }
    close(bs_tcp_fd);
    exit(EXIT_FAILURE);
    /* PARENT */ 
  } else {
    get_central_server_udp(cs_udp_fd, cs_udp_addr, cs_host, cs_addrlen, CSname, CSport); 
    register_backup_server(cs_udp_fd, cs_udp_addr, cs_addrlen);
    close(cs_udp_fd);

    create_backup_server_udp(bs_udp_fd, bs_udp_addr, BSport);
    while(true) {
      char buffer[128] = {0};

      cs_client_addr_len = sizeof(cs_udp_client_addr);
      recvfrom(bs_udp_fd, buffer, sizeof(buffer), 0,
          (struct sockaddr*)&cs_udp_client_addr,
          &cs_client_addr_len);
      std::cout << buffer << std::endl;
    }
    close(bs_udp_fd);
    exit(EXIT_FAILURE);
  }
  return 1;
}