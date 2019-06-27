#include <udp.h>

std::string
recvfrom_with_timeout(int fd, struct sockaddr_in addr, int addrlen, int size) {
  int ret;
  struct timeval tv;
  char buffer[size] = {0};
  std::string msg;

  tv.tv_sec = 2;
  tv.tv_usec = 500000;
  
  if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  ret = recvfrom(fd, buffer, sizeof(buffer), 0,
      (struct sockaddr*)&addr, 
      (socklen_t*)&addrlen);
  if(ret == 0) {
    std::cout << "Timeout: unable to receive udp message.\n";     
  } else if (ret == -1) {
    perror("recvfrom");
    exit(EXIT_FAILURE);
  }
  msg.assign(buffer, strlen(buffer));
  return msg;
}

void
create_backup_server_udp(int &fd, struct sockaddr_in &addr, 
    int port){

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  memset((void*) &addr, (int)'\0', sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((u_short)port);

  if(bind(fd, (struct sockaddr*) &addr,
        sizeof(addr)) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }
  return;
}

void
get_central_server_udp(int &fd, struct sockaddr_in &addr, 
    struct hostent *host, int &addrlen, std::string name, int port){

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  host = gethostbyname(name.c_str());

  memset((void*) &addr,(int)'\0',sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)
      (host->h_addr_list[0]))->s_addr;
  addr.sin_port = htons((u_short)port);

  addrlen = sizeof(addr);
  return;
}

void
get_backup_server_udp(int &fd, std::string ip, std::string port, 
    struct sockaddr_in &addr, int &addrlen){
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  memset((void*) &addr,(int)'\0',sizeof(addr));
  addr.sin_family = AF_INET;
  inet_aton(ip.c_str(),(struct in_addr*) &addr.sin_addr.s_addr);
  addr.sin_port = htons(stoi(port));

  addrlen = sizeof(addr);
  return;
}

void
create_central_server_udp(int &fd, struct sockaddr_in &addr, 
    int port) {
  fd = socket(AF_INET, SOCK_DGRAM, 0);;

  memset((void*) &addr, (int)'\0', sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((u_short)port);

  if(bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }
  return;
}
