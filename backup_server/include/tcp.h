#ifndef TCP_H
#define TCP_H

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
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

std::string read_msg(int, int);
void write_msg(int, std::string);
void write_file(int fd, std::string file, int size);
void read_file(int fd, std::string file, int size);

void create_central_server_tcp(int &fd, struct sockaddr_in &addr, 
    int port);
void connect_to_central_server(int &fd, struct sockaddr_in &addr, 
    struct hostent*host, std::string name, int port);
void create_backup_server_tcp(int &fd, struct sockaddr_in &addr, int port);
void connect_to_backup_server(int &fd, std::string ip, std::string port, struct sockaddr_in &addr);
#endif
