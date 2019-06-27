#include <iostream>
#include <csignal>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <tcp.h>
#include <udp.h>
#include <common.h>
#define PORT_CS 58043
#define PORT_BS 59043

int CSport = 0, BSport = 0;
std::string CSname, active_user;

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
  std::string reg, reply;

  reg = "REG " + get_bs_ip() + " " + std::to_string(BSport) + "\n";
  if(sendto(fd, reg.c_str(), reg.size(), 0,
      (struct sockaddr*)&addr,
      addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  reply = recvfrom_with_timeout(fd, addr, addrlen, 128);
  std::cout << reply;
  return;
}

void
unregister_backup_server(int fd, struct sockaddr_in addr, int addrlen) {
  std::string unreg, reply;

  unreg = "UNR " + get_bs_ip() + " " + std::to_string(BSport) + "\n";
  if(sendto(fd, unreg.c_str(), unreg.size(), 0,
      (struct sockaddr*)&addr,
      addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  reply = recvfrom_with_timeout(fd, addr, addrlen, 128);
  std::cout << reply;
  return;
}

void
exit_backup_server(int signum) {
  get_central_server_udp(cs_udp_fd, cs_udp_addr, cs_host, cs_addrlen, CSname, CSport);
  unregister_backup_server(cs_udp_fd, cs_udp_addr, cs_addrlen);
  close(cs_udp_fd);
  exit(EXIT_SUCCESS);
}

void
exit_backup_child(int signum) {
  close(bs_tcp_fd);
  exit(EXIT_SUCCESS);
}

void
add_user(std::string msg) {
  std::string auth, auth_reply, user;

  auth = msg.substr(4, msg.size()-4);
  user = msg.substr(4, 5);

  write_to_file_append("bs_user_list.txt", auth);
  mkdir(user.c_str(), 0700);

  auth_reply = "LUR OK\n";
  if(sendto(bs_udp_fd, auth_reply.c_str(), auth_reply.size(), 0,
      (struct sockaddr*)&cs_udp_client_addr,
      cs_client_addr_len) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  return;
}

void
get_user_file_list(std::string msg) {
  int space;
  std::ifstream file;
  std::string user, dir, lsf_reply, line, files;

  space = msg.find(" ", 4);
  user = msg.substr(4, space - 4);
  dir = msg.substr(space+1, (msg.size() - (space+1))-1);

  files = get_files(user+"/"+dir);

  lsf_reply = "LFD ";
  lsf_reply.append(files);

  if(sendto(bs_udp_fd, lsf_reply.c_str(), lsf_reply.size(), 0,
      (struct sockaddr*)&cs_udp_client_addr,
      cs_client_addr_len) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  return;
}

void
auth_user() {
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  std::string user, pass;
  std::string response;

  read_msg(client_fd, 1);
  user = read_msg(client_fd, 5);
  read_msg(client_fd, 1);
  pass = read_msg(client_fd, 8);
  read_msg(client_fd, 1);

  response = find_user_and_check_pass("bs_user_list.txt", user, pass);
  active_user = user;
  write_msg(client_fd, response);
  return;
}

void
receive_user_files() {
  int N;
  std::string dir, file_list, new_dir;
  std::string upl_reply;
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);

  dir = read_string(client_fd);
  N = stoi(read_string(client_fd));

  new_dir = active_user+"/"+dir;
  mkdir(new_dir.c_str(), 0700);

  file_list = std::to_string(N) + "\n";
  file_list.clear();

  for(int i = 0; i < N; i++) {
    std::string full_path;
    std::string filename, date, time, size;

    filename = read_string(client_fd);
    date = read_string(client_fd);
    time = read_string(client_fd);
    size = read_string(client_fd);

    full_path = new_dir+"/"+filename;

    read_file(client_fd, full_path, stoi(size));
    change_file_time(full_path, date, time);
    read_msg(client_fd, 1);
    
    std::cout << "Received: " << filename << std::endl;
  }
  std::cout << "Received " << N << " files with success.\n";

  upl_reply = "UPR OK\n";
  write_msg(client_fd, upl_reply);
  return;
}

void
send_user_files() {
  DIR *dir;
  struct dirent *ent;
  struct stat stats;
  std::string files;
  std::string path, dirname, rbr;
  std::string aux, N;

  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);
  dirname = read_string(client_fd);
  rbr = "RBR ";

  path = active_user+"/"+dirname;
  aux = get_files(path);
  int space = aux.find(" ");
  N = aux.substr(0, space);
  rbr.append(N);
  write_msg(client_fd, rbr);

  if((dir = opendir(path.c_str())) != NULL) {
    while((ent = readdir(dir)) != NULL) {
      std::string filename = ent->d_name;
      if(filename.compare(".") && filename.compare("..")) {
        char date_time[20];
        std::string full_path, file_stats, file_size;
        write_msg(client_fd, " ");

        full_path = path + "/" + filename;
        stat(full_path.c_str(), &stats);
        strftime(date_time, 20, "%d.%m.%Y %H:%M:%S",
            localtime(&(stats.st_mtime)));
        file_size = std::to_string(stats.st_size);
        file_stats = filename + " " + date_time + " " + file_size + " ";
        write_msg(client_fd, file_stats);
        std::cout << "Sending: " + filename + "...\n";
        write_file(client_fd, full_path, (int)stats.st_size);
      } 
    }
  }
  write_msg(client_fd, "\n");
  return;
}
void
delete_dir(std::string msg) {
  /*TODO-
   * fazer NOK status_reply
   */
  std::string dirname, user;
  std::string status_reply, path;

  user = msg.substr(4, 5);
  dirname = msg.substr(10, (msg.size() - 1) - 10);

  path = user+"/"+dirname;
  std::cout << "Deleting: " << path << "...\n";
  remove_all(path.c_str());
  status_reply = "DBR OK\n";
  path.clear(); path = user;
  if(is_directory_empty(path.c_str())) {
    std::cout << "Deleting: "<< path << "...\n";
    if(remove(path.c_str()) == -1) {
      status_reply.clear(); status_reply = "DBR NOK\n";
    }
    std::cout << "Removing user: \"" << user << "\"...\n";
    remove_line_from_file_with_key(user, "bs_user_list.txt");
  }

  if(sendto(bs_udp_fd, status_reply.c_str(), status_reply.size(), 0,
      (struct sockaddr*) &cs_udp_client_addr,
      cs_client_addr_len) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
}

int
main(int argc, char **argv) {
  int pid, clientpid;
  std::string protocol;

  if(signal(SIGINT, exit_backup_server) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  parse_input(argc, argv);

  if((pid = fork()) == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
    /* CHILD HANDLES INCOMING CLIENTS */
  } else if (pid == 0) {
    if(signal(SIGINT, exit_backup_child) == SIG_ERR) {
      perror("signal");
      exit(EXIT_FAILURE);
    }
    create_backup_server_tcp(bs_tcp_fd, bs_tcp_addr, BSport);
    while(true) {
      client_len = sizeof(client_addr);
      if((client_fd = accept(bs_tcp_fd, (struct sockaddr*)&client_addr,
          &client_len)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      if((clientpid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
      } else if(clientpid == 0) {
        close(bs_tcp_fd);

        protocol = read_msg(client_fd, 3);
        if(protocol.compare("AUT") == 0) {
          auth_user();

          protocol.clear(); protocol = read_msg(client_fd, 3);
          if(protocol.compare("UPL") == 0) {
            receive_user_files();
          } else if(protocol.compare("RSB") == 0) {
            send_user_files();
          }
        }

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
      if(recvfrom(bs_udp_fd, buffer, sizeof(buffer), 0,
          (struct sockaddr*)&cs_udp_client_addr,
          &cs_client_addr_len) == -1) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
      }

      if(strncmp(buffer, "LSF", 3) == 0) {
        get_user_file_list(buffer);
      } else if(strncmp(buffer, "LSU", 3) == 0) {
        add_user(buffer);
      } else if(strncmp(buffer, "DLB", 3) == 0) {
        delete_dir(buffer);
      }
    }
    close(bs_udp_fd);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_FAILURE);
}
