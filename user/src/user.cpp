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
#include <tcp.h>
#include <common.h>
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
logout() {
  logged = false;
  auth_str.clear();
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
    } else {
      std::cout << "[AUR-ERR] Unexpected protocol message.\n";
    }
    close(cs_tcp_fd);
  } else {
    std::cout << "[WARNING] User: \"" + auth_str.substr(4, 5) + "\" is logged in.\n";
  }
  return;
}

void
deluser() {
  std::string dlu, dlu_reply;
  std::string user, auth_reply;

  if(logged) {
    user = auth_str.substr(4, 5);
    dlu = "DLU\n";

    connect_to_central_server(cs_tcp_fd, cs_tcp_addr, cs_host,
        CSname, CSport);
    write_msg(cs_tcp_fd, auth_str);

    auth_reply = read_msg(cs_tcp_fd, 3);
    if(auth_reply.compare("AUR") == 0) {
      read_msg(cs_tcp_fd, 1);
      auth_reply.clear(); auth_reply = read_msg(cs_tcp_fd, 3);

      if(auth_reply.compare("OK\n") == 0) {
        write_msg(cs_tcp_fd, dlu);
        dlu_reply = read_msg(cs_tcp_fd, 3);

        if(dlu_reply.compare("DLR") == 0) {
          read_msg(cs_tcp_fd, 1);
          dlu_reply.clear(); dlu_reply = read_msg(cs_tcp_fd, 3);

          if(dlu_reply.compare("OK\n") == 0) {
            std::cout << "User: \"" + user + "\" was deleted with success.\n";
            logout();
            close(cs_tcp_fd);
          } else if(dlu_reply.compare("NOK") == 0) {
            read_msg(cs_tcp_fd, 1);
            std::cout << "Unable to delete user: \"" +
              user + "\", because user still has information stored.\n";
            close(cs_tcp_fd);
          }
        }
      } else if(auth_reply.compare("NOK") == 0) {
        read_msg(cs_tcp_fd, 1);
        close(cs_tcp_fd);
        std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
      }
    } else {
      std::cout << "[AUR-ERR] Unexpected protocol message.\n";
      close(cs_tcp_fd);
      exit(EXIT_FAILURE);
    }
  } else {
    std::cout << "[WARNING] No user logged-in.\n";
  }
  return;
}


std::string
process_files_reply(std::string &ip, std::string &port, int &N) {
  int i;
  std::string files;
  std::string inc;

  read_msg(cs_tcp_fd, 1);

  inc = read_msg(cs_tcp_fd, 4);
  if(inc.compare("NOK\n") == 0)
    return "NOK\n";

  ip = inc;
  ip.append(read_string(cs_tcp_fd));
  port = read_string(cs_tcp_fd);
  N = stoi(read_string(cs_tcp_fd));

  for(i = 0; i < N; i++) {
    std::string line;
    std::string filename, date, time, size;

    filename = read_string(cs_tcp_fd);
    date = read_string(cs_tcp_fd);
    time = read_string(cs_tcp_fd);
    size = read_string(cs_tcp_fd);

    line = std::to_string(i+1)+": " +date+" "+time+" "+size+ " " +filename +"\n";
    files.append(line);
  }
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
        upl = "UPL " + dir + " " + std::to_string(N);
        write_msg(bs_tcp_fd, upl);

        for(int i = 0; i < N; i++) {
          std::string line;
          std::string filename, date, time, size;

          write_msg(bs_tcp_fd, " ");

          filename = read_string(cs_tcp_fd);
          date = read_string(cs_tcp_fd);
          time = read_string(cs_tcp_fd);
          size = read_string(cs_tcp_fd);

          line = filename+" "+date+" "+time+" "+size + " ";
          std::cout << "Uploading: " << filename << "...\n";
          write_msg(bs_tcp_fd, line);
          write_file(bs_tcp_fd, dir+"/"+filename, stoi(size));
        }
        close(cs_tcp_fd);
        write_msg(bs_tcp_fd, "\n");
      } else {
        if(auth_reply.compare("NOK") == 0) {
          read_msg(bs_tcp_fd, 1);
          std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
          close(bs_tcp_fd);
        }
      }
    } else {
      std::cout << "[AUR-ERR] Unexpected protocol message.\n";
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
        std::string aux = get_files(dirname);
        file_list = "BCK " + dirname + " ";
        file_list.append(aux);
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
    } else {
      std::cout << "[AUR-ERR] Unexpected protocol message\n";
    }
  } else {
    std::cout << "[WARNING] No available session to backup from.\n";
  }
  return;
}

void
download_files_from_bs(std::string dirname) {
  int N, i;
  std::string filename, date, time, size;

  read_msg(bs_tcp_fd, 1);
  N = stoi(read_string(bs_tcp_fd));

  for(i = 0; i < N; i++) {
    filename = read_string(bs_tcp_fd);
    date = read_string(bs_tcp_fd);
    time = read_string(bs_tcp_fd);
    size = read_string(bs_tcp_fd);

    std::string path = dirname+"/"+filename;
    read_file(bs_tcp_fd, path, stoi(size));
    read_msg(bs_tcp_fd, 1);
    std::cout << "Received: " + filename +"\n";
  }
  return;
}

void
restore() {
  std::string rst, rst_reply;
  std::string auth_reply, dirname;
  std::string bs_ip, bs_port;
  std::string rsb, rsb_reply;

  std::cin >> dirname;

  if(logged) {
    connect_to_central_server(cs_tcp_fd, cs_tcp_addr, cs_host,
        CSname, CSport);
    write_msg(cs_tcp_fd, auth_str);

    auth_reply = read_msg(cs_tcp_fd, 3);
    if(auth_reply.compare("AUR") == 0) {
      read_msg(cs_tcp_fd, 1);

      auth_reply.clear(); auth_reply = read_msg(cs_tcp_fd, 3);
      if(auth_reply.compare("OK\n") == 0) {
        rst = "RST " + dirname + "\n";
        write_msg(cs_tcp_fd, rst);

        rst_reply = read_msg(cs_tcp_fd, 3);
        if(rst_reply.compare("RSR") == 0) {
          read_msg(cs_tcp_fd, 1);

          rst_reply.clear(); rst_reply = read_msg(cs_tcp_fd, 4);
          if(rst_reply.compare("EOF\n") == 0) {
            close(cs_tcp_fd);
            std::cout <<"[RSR-EOF] Restore request cannot be answered.\n";
          } else if(rst_reply.compare("ERR\n") == 0) {
            close(cs_tcp_fd);
            std::cout << "[RSR-ERR] \n";
          } else {
            bs_ip = rst_reply;
            bs_ip.append(read_string(cs_tcp_fd));
            bs_port = read_string(cs_tcp_fd);

            std::cout << "Restore from: " << bs_ip << " " << bs_port << std::endl;

            close(cs_tcp_fd);
            connect_to_backup_server(bs_tcp_fd, bs_ip, bs_port, bs_tcp_addr);
            write_msg(bs_tcp_fd, auth_str);

            auth_reply.clear(); auth_reply = read_msg(bs_tcp_fd, 3);
            if(auth_reply.compare("AUR") == 0) {
              read_msg(bs_tcp_fd, 1);
              auth_reply.clear(); auth_reply = read_msg(bs_tcp_fd, 3);
              if(auth_reply.compare("OK\n") == 0) {
                rsb = "RSB " + dirname +"\n";
                write_msg(bs_tcp_fd, rsb);

                rsb_reply = read_msg(bs_tcp_fd, 3);
                if(rsb_reply.compare("RBR") == 0) {
                  mkdir(dirname.c_str(), 0700);
                  download_files_from_bs(dirname);
                  close(bs_tcp_fd);
                }
              } else if(auth_reply.compare("NOK") == 0) {
                read_msg(bs_tcp_fd, 1);
                close(bs_tcp_fd);
                std::cout << "[AUR-NOK] Auth was unsuccessful.\n";
              }
            }
          }  //ISTO É HORRIVEL
        }
      } else if(auth_reply.compare("NOK") == 0) {
        read_msg(cs_tcp_fd, 1);
        close(cs_tcp_fd);
        std::cout << "[AUR-NOK] Auth was unsuccess.\n";
      }
    } else {
      std::cout << "[ERR] Something went terrible wrong.\n";
      close(cs_tcp_fd);
    }
  } else {
    std::cout << "[WARNING] No available session.\n";
  }
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
          std::cout << "User doesn't have any directories stored.\n";
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
  } else {//assim o user app fecha
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
          if(files_resp.compare("NOK\n") == 0) {
            std::cout << "List files request cannot be answered by cs.\n";
          } else {
            close(cs_tcp_fd);
            std::cout << "from: " + bs_ip +" "+bs_port+"\n"+std::to_string(N)+" files found: \n";
            std::cout << files_resp;
          }
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
delete_dir() {
  std::string dirname, auth_reply, protocol, status;
  std::cin >> dirname;

  if (logged) {
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

        protocol = "DEL " + dirname + "\n";
        write_msg(cs_tcp_fd, protocol);

        status = read_msg(cs_tcp_fd, 3);
        if(status.compare("DDR") == 0) {
          read_msg(cs_tcp_fd, 1);
          status.clear(); status = read_msg(cs_tcp_fd, 3);

          if(status.compare("OK\n") == 0) {
            std::cout << "Directory \"" +dirname+ "\" was deleted with success.\n";
          } else if(status.compare("NOK") == 0) {
            read_msg(cs_tcp_fd, 1);
            std::cout << "Unable to delete \""+dirname+"\".\n";
          } else if(status.compare("ERR") == 0) {
            read_msg(cs_tcp_fd, 1);
            std::cout << "[DDR-ERR] TODO\n";
          }
        } else {
          std::cout << "[AUR-ERR] TODO\n";
        }
      }
    }
    close(cs_tcp_fd);
  }
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
      delete_dir();
    } else if(input.compare("logout") == 0) {
      logout();
    } else if(input.compare("exit") == 0) {
      exit(EXIT_SUCCESS);
    } else {
      std::cout << "[ERR] Unknown command: \"" + input + "\"\n";
    }
  }
  exit(EXIT_FAILURE);
}
