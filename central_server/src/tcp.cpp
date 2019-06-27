#include <tcp.h>

std::string
read_msg(int fd, int nbytes) {
  int nleft, nread = 0;
  char buffer[nbytes+1] = {0};
  char *ptr;
  
  ptr = buffer;
  nleft = nbytes;
  while(nleft > 0) {
    if((nread = read(fd, ptr, nleft)) == -1) {
      fprintf(stderr, "read: error");
      exit(EXIT_FAILURE);
    } else if (nread == 0) {
      break;
    }
    nleft -= nread;
    ptr += nread;
  } 
  buffer[nbytes+1] = '\0';
  return buffer;
}

void
write_msg(int fd, std::string msg) {
  int nleft, nwritten = 0;
  char *ptr; 

  nleft = msg.size();
  char buffer[nleft];
  strncpy(buffer, msg.c_str(), nleft);
  ptr = buffer;
  while(nleft > 0) {
    if((nwritten = write(fd, ptr, nleft)) <= 0) {
      fprintf(stderr, "write: write_msg(int, std::string)\n");
      exit(EXIT_FAILURE);
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return;
}

void
write_file(int fd, std::string file, int size) {
  int filefd;
  int nleft, nread = 0, nwritten = 0;
  char buffer[size+1];
  char *ptr;

  if((filefd = open(file.c_str(), O_RDONLY)) == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  ptr = buffer;
  nleft = size;
  while(nleft > 0) {
    if((nread = read(filefd, ptr, nleft)) == -1) {
      fprintf(stderr, "read_file: could not read file.\n");
      exit(EXIT_FAILURE);
    } else if (nread == 0) {
      break;
    } 
    nleft -= nread;
    ptr += nread;
  }
  close(filefd);

  ptr = buffer;
  nleft = size;
  while(nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) <= 0) {
      fprintf(stderr, "write: write_file\n");
      exit(EXIT_FAILURE);
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return;
}

void
read_file(int fd, std::string file, int size) {
  int filefd;
  int nleft, nread = 0, nwritten = 0;
  char buffer[size+1];
  char *ptr;

  ptr = buffer;
  nleft = size;
  while(nleft > 0) {
    if((nread = read(fd, ptr, nleft)) == -1) {
      fprintf(stderr, "read_file: could not read file.\n");
      exit(EXIT_FAILURE);
    } else if (nread == 0) {
      break;
    } 
    nleft -= nread;
    ptr += nread;
  }
  
  if((filefd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR)) == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  ptr = buffer;
  nleft = size;
  while(nleft > 0) {
     if((nwritten = write(filefd, ptr, nleft)) <= 0) {
      fprintf(stderr, "write: write_file\n");
      exit(EXIT_FAILURE);
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  close(filefd);
  return;
}

void
create_central_server_tcp(int &fd, struct sockaddr_in &addr, int port) {
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset((void*) &addr, (int)'\0',sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((u_short)port);

  if (bind(fd, (struct sockaddr*) &addr,
        sizeof(addr)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(fd, 5) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  return;
}

void
connect_to_central_server(int &fd, struct sockaddr_in &addr, 
    struct hostent* host, std::string name, int port) {
   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if ((host = gethostbyname(name.c_str())) == NULL) {
    perror("gethostbyname");
    exit(EXIT_FAILURE);
  }

  memset((void*)&addr, (int)'\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr*)
      (host->h_addr_list[0]))->s_addr;
  addr.sin_port = htons((u_short)port);

  if (connect(fd, (struct sockaddr*)&addr,
        sizeof(addr)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  return;
}

void
create_backup_server_tcp(int &fd, struct sockaddr_in &addr,
    int port) {
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset((void*) &addr, (int)'\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((u_short)port);

  if(bind(fd, (struct sockaddr*) &addr,
        sizeof(addr)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if(listen(fd, 5) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  return;
}

void
connect_to_backup_server(int &fd, std::string ip, std::string port,
    struct sockaddr_in &addr) {
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset((void*)&addr, (int)'\0', sizeof(addr));
  addr.sin_family = AF_INET;
  inet_aton(ip.c_str(), 
      (struct in_addr*)&addr.sin_addr.s_addr);
  addr.sin_port = htons((u_short)stoi(port));

  if(connect(fd, (struct sockaddr*)&addr,
        sizeof(addr)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  return;
}
