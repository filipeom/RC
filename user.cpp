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

/* CS SERVER TCP */
int cs_tcp_fd;
struct hostent *cs_host;
struct sockaddr_in cs_tcp_addr;

/* BS SERVER TCP */
int bs_tcp_fd;
struct sockaddr_in bs_tcp_addr;

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
        fprintf(stderr,"[ERR] Usage: ./user [-n CSname] [-p CSport]\n");
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
        std::cout << "[AUR-NOK] Auth was unsuccessful." << std::endl;
        read_msg(cs_tcp_fd, 1);
        auth_reply.clear();
        logged = false;
      } else {
        std::cout << "User \"" +user +"\" is now logged-in.\n";
        logged = true;
      }
    }
    close(cs_tcp_fd);
  } else {
    std::cout << "[WARNING] User: \"" + auth_str.substr(4, 5) + "\" is logged in.\n";
  }
  return;
}

void
deluser() {
  return;
}


std::string
process_files_reply(std::string &ip, std::string &port, int &N) {
  int i;
  std::string files;
  read_msg(cs_tcp_fd, 1);

  ip = read_string(cs_tcp_fd);
  port = read_string(cs_tcp_fd);
  N = stoi(read_string(cs_tcp_fd));

  for(i = 0; i < N; i++) {
    std::string line;
    std::string filename, date, time, size;

    filename = read_string(cs_tcp_fd);
    date = read_string(cs_tcp_fd);
    time = read_string(cs_tcp_fd);
    size = read_string(cs_tcp_fd);

    line = filename+" "+date+" "+time+" "+size+" ";
    files.append(line);
  }
  //READ TRAILING \n
  read_msg(cs_tcp_fd, 1);
  files.append("\n");
  return files;
}

void
receive_updated_file_list_and_send_files(std::string dir) {
  int N;
  std::string auth_reply, upl;
  std::string ip, port, bck_reply;

  bck_reply = read_msg(cs_tcp_fd, 4);
  if(bck_reply.compare("EOF\n") == 0) {
    std::cout << "[BKR-EOF] No Backup Server available to backup.\n";
  } else if (bck_reply.compare("ERR\n") == 0) {
    std::cout << "[BKR-ERR] Backup request is not correctly formulated.\n"; 
  } else {
    ip = bck_reply;
    ip.append(read_string(cs_tcp_fd));
    port = read_string(cs_tcp_fd);

    std::cout << "Backup to: " + ip + " " + port <<std::endl;

    connect_to_backup_server(bs_tcp_fd, ip, port, bs_tcp_addr);
    write_msg(bs_tcp_fd, auth_str);

    auth_reply = read_msg(bs_tcp_fd, 3);
    if(auth_reply.compare("AUR") == 0) {
      read_msg(bs_tcp_fd, 1);
      auth_reply.clear(); auth_reply = read_msg(bs_tcp_fd, 3);

      if(auth_reply.compare("OK\n") == 0) {
        N = stoi(read_string(cs_tcp_fd));
        upl = "UPL " + dir + " " + std::to_string(N) + " ";
        write_msg(bs_tcp_fd, upl);

        for(int i = 0; i < N; i++) {
          std::string line;
          std::string filename, date, time, size;

          filename = read_string(cs_tcp_fd);
          date = read_string(cs_tcp_fd);
          time = read_string(cs_tcp_fd);
          size = read_string(cs_tcp_fd);

          line = filename+" "+date+" "+time+" "+size+" ";
          std::cout << "Uploading: " << filename << "...\n";
          write_msg(bs_tcp_fd, line);
          write_file(bs_tcp_fd, dir+"/"+filename, stoi(size));
        }
        read_msg(cs_tcp_fd, 1);
        close(cs_tcp_fd);
        write_msg(bs_tcp_fd, "\n");
      } else {
        if(auth_reply.compare("NOK") == 0) {
          read_msg(bs_tcp_fd, 1);
          std::cout << "[AUR-NOK] BS Auth was unsuccessful.\n" << std::endl;
          close(bs_tcp_fd);
        }
      }
    }
  }
  return;  
}

void
backup() {
  std::string auth_reply, bck_reply, upl_reply;
  std::string dirname, file_list;
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
        /* This string ends with: " \n" */
        file_list = get_files(dirname);
        /* Send BCK msg*/
        write_msg(cs_tcp_fd, file_list);
        bck_reply = read_msg(cs_tcp_fd, 3);

        if(bck_reply.compare("BKR") == 0) {
          read_msg(cs_tcp_fd, 1);    
          receive_updated_file_list_and_send_files(dirname);
          upl_reply = read_msg(bs_tcp_fd, 3);

          if(upl_reply.compare("UPR") == 0) {
            read_msg(bs_tcp_fd, 1);
            upl_reply.clear(); upl_reply = read_msg(bs_tcp_fd, 3);

            if(upl_reply.compare("OK\n") == 0) {
              std::cout << "Files Uploaded Succesfully." << std::endl;
              close(bs_tcp_fd);
            } else if(upl_reply.compare("NOK") == 0) {
              read_msg(bs_tcp_fd, 1);
              close(bs_tcp_fd);
              std::cout << "[UPR-NOK] Backup request was unsuccessful.\n";
            }
          }
        }
      } else if(auth_reply.compare("NOK") == 0) {
        read_msg(cs_tcp_fd, 1);
        std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
        close(cs_tcp_fd);
      }
    }
  } else {
    std::cout << "[WARNING] No available session to backup from.\n";
  }
  return;
}

void
restore() {
  return;
}

void
dirlist() {
  std::string dirlist, protocol, auth_reply;
  std::string dirname;
  int N, cont;

  if(logged) {
    connect_to_central_server(cs_tcp_fd, cs_tcp_addr, cs_host,
        CSname, CSport);

    write_msg(cs_tcp_fd, auth_str);
    auth_reply = read_msg(cs_tcp_fd, 3);

    if(auth_reply.compare("AUR") == 0) {
      read_msg(cs_tcp_fd, 1);
      auth_reply.clear(); auth_reply = read_msg(cs_tcp_fd, 3);

      if(auth_reply.compare("NOK") == 0) {
        read_msg(cs_tcp_fd, 1);
        std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
        close(cs_tcp_fd);
        return;
      } else if(auth_reply.compare("OK\n") == 0){

        protocol = "LSD\n";
        write_msg(cs_tcp_fd, protocol);
        protocol.clear(); protocol = read_msg(cs_tcp_fd, 3);
        read_msg(cs_tcp_fd, 1);

        N = stoi(read_string(cs_tcp_fd));

        if(N == 0){
          std::cout << "[LSD] Unable to list user directories.\n";
          close(cs_tcp_fd);
          return;
        }
        cont = N; 
        while(cont != 0) {
          dirname = read_string(cs_tcp_fd);
          dirlist.append(dirname + " ");
          cont -= 1;
        }
        std::cout << dirlist+"\n";

        close(cs_tcp_fd);
        return;
      }
    }
  } else {
    std::cout << "[WARNING] No available session.\n";
  }
  /* Should never reach */
  close(cs_tcp_fd);
  exit(EXIT_FAILURE);
}

void
filelist() {
  int N = 0;
  std::string auth_reply, cs_reply;
  std::string dirname;
  std::string bs_ip, bs_port, files_resp;
  std::cin >> dirname;
  std::string msg;

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
        msg = "LSF " + dirname + "\n";
        write_msg(cs_tcp_fd, msg);
        //PAULO AQUI
        cs_reply = read_msg(cs_tcp_fd, 3);
        if(cs_reply.compare("LFD") == 0) {
          files_resp = process_files_reply(bs_ip, bs_port, N);
          close(cs_tcp_fd);
          std::cout << bs_ip<<" "<<bs_port<<" "<<N<<" "<<files_resp;
        }
      } else if(auth_reply.compare("NOK") == 0){
        read_msg(cs_tcp_fd, 1);
        std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
      }
    }
  } else {
    std::cout << "[WARNING] No available session.\n";
  }
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
