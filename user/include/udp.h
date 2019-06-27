#ifndef UDP_H
#define UDP_H

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

std::string recvfrom_with_timeout(int fd, struct sockaddr_in addr, int addrlen, int size);
void create_backup_server_udp(int &fd, struct sockaddr_in &addr, 
    int port);
void get_backup_server_udp(int &fd, std::string ip, std::string port, 
    struct sockaddr_in &addr, int &addrlen);
void create_central_server_udp(int &fd, struct sockaddr_in &addr,
    int port);
void get_central_server_udp(int &fd, struct sockaddr_in &addr,
    struct hostent *host, int &addrlen, std::string name, int port);
#endif
