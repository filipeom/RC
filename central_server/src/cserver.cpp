#include <iostream>
#include <cstdlib>
#include <ctime>
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
#include <tcp.h>
#include <udp.h>
#include <common.h>
#define PORT 58043

int CSport = 0;
std::string active_user;

/* CS TCP SERVER */
int cs_tcp_fd;
struct sockaddr_in cs_tcp_addr;

/* CS UDP SERVER */
int cs_udp_fd;
struct sockaddr_in cs_udp_addr;

/*BS UDP SERVER */
int bs_udp_fd;
struct sockaddr_in bs_udp_addr;
int bs_addrlen;

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

void
delete_user_dir_from_file(std::string user, std::string dir) {
  std::ifstream ifile;
  std::ofstream ofile;
  std::string line;

  ifile.open("backup_list.txt");
  ofile.open("temp.txt");

  while(std::getline(ifile, line)) {
    std::string username, dirname;
    int space, length;

    username = line.substr(0, 5);
    space = line.find(" ");
    space = line.find(" ", space + 1);
    space = line.find(" ", space + 1);
    length = line.size() - 1 - space;
    dirname = line.substr(space + 1, length );

    if(!((username.compare(user) == 0) && (dirname.compare(dir) == 0)))
      ofile << line << std::endl;
  }
  ofile.close();
  ifile.close();
  remove("backup_list.txt");
  rename("temp.txt","backup_list.txt");
  return;
}

bool
check_if_bs_exists(std::string file, std::string ip,
    std::string port) {
  std::string line;
  std::ifstream ifile;

  ifile.open(file, std::ios::in);

  while(std::getline(ifile, line)) {
    std::string bs_ip, bs_port;
    int space;

    space = line.find(" ");
    bs_ip = line.substr(0, space);
    bs_port = line.substr(space + 1, line.size() - (space + 1));

    if((ip.compare(bs_ip) == 0) && (port.compare(bs_port) == 0)) {
      ifile.close();
      return true;
    }
  }
  ifile.close();
  return false;
}

void
register_backup_server(int fd, struct sockaddr_in addr,
    socklen_t addrlen, std::string str) {
  int space1;
  int space2;
  char dest[16] = {0};
  std::string ip, port, reply;

  space1 = str.find(" ");
  space2 = str.find(" ", space1+1);
  ip = str.substr(space1 + 1, space2 - (space1 + 1));
  port = str.substr(space2 + 1, (str.size()-1) - (space2+1));

  if((inet_pton(AF_INET, ip.c_str(), dest) == 0) || !is_number(port)) {
    reply = "RGR ERR\n";
  }
  else if(check_if_bs_exists("bs_list.txt", ip, port)) {
    reply = "RGR NOK\n";
  } else {
    write_to_file_append("bs_list.txt", ip + " " + port+"\n");
    std::cout << "+BS: " + ip + " " + port << std::endl;
    reply = "RGR OK\n";
  }

  if(sendto(fd, reply.c_str(), reply.size(), 0,
      (struct sockaddr*)&addr,
      addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  return;
}


void
unregister_backup_server(int fd, struct sockaddr_in addr,
    socklen_t addrlen, std::string str) {
  std::string ip, port, reply = "";
  std::string line, search_string;
  std::ifstream ifile;
  std::ofstream ofile;
  int space1, space2;
  char dest[16] = {0};

  space1 = str.find(" ");
  space2 = str.find(" ", space1+1);
  ip = str.substr(space1 + 1, space2 - (space1 + 1));
  port = str.substr(space2 + 1, (str.size()-1) - (space2+1));

  search_string = ip + " " + port;

  if((inet_pton(AF_INET, ip.c_str(), dest) == 0) || !is_number(port)) {
    reply = "UAR ERR\n";
    if(sendto(fd, reply.c_str(), reply.size(), 0,
        (struct sockaddr*) &addr, addrlen) == -1) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
    return;
  }

  ifile.open("bs_list.txt");
  ofile.open("temp.txt");
  while(std::getline(ifile, line)) {
    if(line.compare(search_string) != 0) {
      ofile << line << std::endl;
    } else {
      reply.clear(); reply = "UAR OK\n";
    }
  }
  ofile.close();
  ifile.close();
  remove("bs_list.txt");
  rename("temp.txt", "bs_list.txt");
  if(reply.empty()) {
    reply.clear(); reply = "UAR NOK\n";
  }

  if(sendto(fd, reply.c_str(), reply.size(), 0,
      (struct sockaddr*) &addr, addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
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

  response = find_user_and_check_pass("cs_user_list.txt", user, pass);
  active_user.clear(); active_user = user;
  write_msg(client_fd, response);
  return;
}


void
delete_user() {
  std::string dlu_reply;
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);
  if(check_if_string_exists_in_file(active_user, "backup_list.txt")) {
    dlu_reply = "DLR NOK\n";
    write_msg(client_fd, dlu_reply);
  } else {
    remove_line_from_file_with_key(active_user, "cs_user_list.txt");
    dlu_reply = "DLR OK\n";
    write_msg(client_fd, dlu_reply);
  }
  return;
}

std::string
read_file_list(std::string &dir, int &N) {
  int i;
  std::string file_list;

  dir = read_string(client_fd);
  N = stoi(read_string(client_fd));

  for(i = 0; i < N; i++) {
    std::string line;
    std::string filename, date, time, size;

    filename = read_string(client_fd);
    date = read_string(client_fd);
    time = read_string(client_fd);
    size = read_string(client_fd);

    line = " " + filename+" "+date+" "+time+" "+size;
    file_list.append(line);
  }
  file_list.append("\n");
  return file_list;
}

bool
find_user_dir(std::string dir, std::string user, std::string &ip,
    std::string &port) {
  std::string line;
  std::ifstream file;

  file.open("backup_list.txt");
  while(std::getline(file, line)) {
    std::size_t found = line.find(user);
    if(found != std::string::npos) {
      found = line.find(dir);
      if(found != std::string::npos) {
        int space1, space2, space3;

        space1 = line.find(" ");
        space2 = line.find(" ", space1 + 1);
        space3 = line.find(" ", space2 + 1);

        ip = line.substr(space1 + 1, (space2 - space1)-1);
        port = line.substr(space2 + 1, (space3 - space2)-1);
        file.close();
        return true;
      }
    }
  }
  return false;
}

bool
find_user_bs(std::string user, std::string &ip, std::string &port) {
  std::string line;
  std::ifstream file;

  file.open("backup_list.txt");
  while(std::getline(file, line)) {
    std::size_t found = line.find(user);
    if(found != std::string::npos) {
      int space1, space2, space3;

      space1 = line.find(" ");
      space2 = line.find(" ", space1 + 1);
      space3 = line.find(" ", space2 + 1);

      ip = line.substr(space1 + 1, (space2 - space1)-1);
      port = line.substr(space2 + 1, (space3 - space2)-1);
      file.close();
      return true;
    }
  }
  return false;
}

int get_number_of_BSs(){
  int num = 0;
  std::string line;
  std::ifstream file;

  file.open("bs_list.txt");
  while(std::getline(file, line)){
    num++;
  }

  return num;
}

void
get_bs_for_user(std::string &ip, std::string &port) {
  int space;
  std::string line;
  std::ifstream file;
  int num_linhas = get_number_of_BSs();
  srand(time(NULL));
  int random_number = rand() % num_linhas + 1;

  file.open("bs_list.txt");
  for(int i = 0; i < random_number; i++){
      std::getline(file, line);
  }
  file.close();
  space = line.find(" ");
  ip = line.substr(0, space);
  port = line.substr(space+1, line.size() - (space+1));
  return;
}

void
register_user_in_bs(std::string msg, std::string ip,
    std::string port) {
  std::string reply, protocol;

  protocol = "LSU " + msg+"\n";
  get_backup_server_udp(bs_udp_fd, ip, port,
      bs_udp_addr, bs_addrlen);
  if(sendto(bs_udp_fd, protocol.c_str(), protocol.size(), 0,
      (struct sockaddr*)&bs_udp_addr,
      bs_addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  reply = recvfrom_with_timeout(bs_udp_fd, bs_udp_addr, bs_addrlen, 128);
  std::cout << reply;
  close(bs_udp_fd);
  return;
}

void
send_client_bs_and_file_list(std::string ip, std::string port,
    int N, std::string file_list) {
  std::string msg;

  msg = "BKR "+ip+" "+port+" "+std::to_string(N);
  msg.append(file_list);
  write_msg(client_fd, msg);
  return;
}

std::string
ask_bs_for_files(std::string dir, std::string user, std::string ip,
    std::string port) {
  std::string protocol, reply;

  protocol = "LSF " + user + " " + dir +"\n";
  get_backup_server_udp(bs_udp_fd, ip, port,
      bs_udp_addr, bs_addrlen);
  if(sendto(bs_udp_fd, protocol.c_str(), protocol.size(), 0,
      (struct sockaddr*)&bs_udp_addr,
      bs_addrlen) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  reply = recvfrom_with_timeout(bs_udp_fd, bs_udp_addr, bs_addrlen, 2048);
  close(bs_udp_fd);
  return reply;
}

std::string
update_file_list(int N, int &new_N, std::string file_lst1, std::string file_lst2) {
  std::string name1, data1, time1, size1;
  std::string name2, data2, time2, size2;
  std::string update_file_list, line;

  std::size_t front2, back2;
  std::size_t front1, back1;
  std::size_t safe_pos;

  bool copy = false;
  bool diff_names = true;
  bool break1 = false;
  bool break2 = false;

  front2 = file_lst2.find(" ");
  
  safe_pos = front2;
  back2 = front2;
  front2 += 1;

  front1 = 0;
  back1 = 0;
  while(true) {

    if(break1){
      break;
    }

    front1 = file_lst1.find(" ", front1);
    name1 = file_lst1.substr(back1, front1 - back1);

    front1 += 1;
    back1  = front1;

    front1 = file_lst1.find(" ", front1);
    data1  = file_lst1.substr(back1, front1 - back1 );

    front1 += 1;
    back1  = front1;

    front1 = file_lst1.find(" ", front1);
    time1  = file_lst1.substr(back1, front1 - back1);

    front1 += 1;
    back1  = front1;

    front1 = file_lst1.find(" ", front1);
    if(front1 == std::string::npos){

      size1 = file_lst1.substr(back1, file_lst1.size() - 1 - back1);
      break1 = true;

    } else {
      size1  = file_lst1.substr(back1, front1 - back1);
    }

    front1 += 1;
    back1 = front1;

    break2 = false;
    front2 = safe_pos;
    back2 = front2;
    front2 += 1;


    while(true) {

      if(break2) {
        break;
      }

      front2 = file_lst2.find(" ", front2);
      name2 = file_lst2.substr(back2+1, front2 - back2 - 1 );

      back2 = front2;
      front2 += 1;

      front2 = file_lst2.find(" ", front2);
      data2 = file_lst2.substr(back2+1, front2 - back2 - 1);

      back2 = front2;
      front2 += 1;

      front2 = file_lst2.find(" ", front2);
      time2 = file_lst2.substr(back2+1, front2 - back2 - 1);

      back2 = front2;
      front2 += 1;

      front2 = file_lst2.find(" ", front2);
      if(front2 == std::string::npos){

        size2 = file_lst2.substr(back2 + 1, file_lst2.size() - 1 - back2 - 1);
        break2 = true;
      } else {
        size2 = file_lst2.substr(back2 + 1, front2 - back2 - 1);
      }

      back2 = front2;
      front2 += 1;

      /*COMPARACOES*/
      if(name1.compare(name2) == 0) {
        diff_names = false;
        if((data1.compare(data2) != 0) || (time1.compare(time2) != 0)){
          copy = true;
        }
      }
    }
    if(copy || diff_names) {
      new_N += 1;
      line.clear();
      line = " " + name1 + " " + data1 + " " + time1 + " " + size1;
      update_file_list.append(line);
    }
    copy = false;
    diff_names = true;
  }
  update_file_list.append("\n");
  return update_file_list;
}

void
backup_user_dir() {
  int N, new_N = 0;
  std::string dirname, file_list;
  std::string bs_ip, bs_port;
  std::string bs_file_lst, updated_files;
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);

  file_list = read_file_list(dirname, N);
  if(find_user_dir(dirname, active_user, bs_ip, bs_port)) {
    bs_file_lst = ask_bs_for_files(dirname, active_user, bs_ip, bs_port);
    updated_files = update_file_list(N, new_N, file_list.substr(1, file_list.size()), bs_file_lst.substr(4, bs_file_lst.size()));
    send_client_bs_and_file_list(bs_ip, bs_port, new_N, updated_files);
    return;
  } else if(find_user_bs(active_user, bs_ip, bs_port)) {
    //IF USER IS REGISTERED IN A BS SEND FILES THERE
  } else {
    get_bs_for_user(bs_ip, bs_port);
    register_user_in_bs(find_string(active_user, "cs_user_list.txt"), bs_ip, bs_port);
  }
  write_to_file_append("backup_list.txt",
      active_user+" "+bs_ip+ " "+bs_port+" "+dirname+"\n");
  send_client_bs_and_file_list(bs_ip, bs_port, N, file_list);
  return;
}

void
restore_user_dir() {
  std::string auth_reply, rst_reply;
  std::string dirname, bs_ip, bs_port;

  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);

  dirname = read_string(client_fd);
  if(find_user_dir(dirname, active_user, bs_ip, bs_port)) {
    rst_reply = "RSR " + bs_ip + " " + bs_port + "\n";
  } else {
    rst_reply = "RSR EOF\n";
  }
  write_msg(client_fd, rst_reply);
  return;
}

void
dir_list() {
  std::ifstream file;
  std::string directories, line, reply;
  int N = 0;

  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd,1);
  file.open("backup_list.txt");

  while(std::getline(file, line)) {
    std::string username;
    username = line.substr(0,5);
    if(username.compare(active_user) == 0) {
      int space1, space2, length;
      std::string directory_name;

      space1 = line.find(" ");
      space1 = line.find(" ", space1 + 1);
      space1 = line.find(" ", space1 + 1);
      space2 = line.find(" ", space1 + 1);
      length = (space2 - 1) - space1;

      directory_name = line.substr(space1 + 1, length);
      directories.append(" " + directory_name);
      N += 1;
    }
  }
  if(N == 0){
    reply = "LDR 0\n";
    write_msg(client_fd, reply);
    return;
  }
  reply = "LDR " + std::to_string(N) + directories + "\n";
  write_msg(client_fd, reply);
  return;
}

void
file_list() {
  std::string lsf, lsf_reply, dir;
  std::string ip, port, files;

  read_msg(client_fd, 1);
  dir = read_string(client_fd);
  if(find_user_dir(dir, active_user, ip, port)) {
    find_user_dir(dir, active_user, ip, port);
    files = ask_bs_for_files(dir, active_user, ip, port);
    lsf_reply = "LFD " + ip + " " + port + files.substr(3, files.size());
    write_msg(client_fd, lsf_reply);
  } else {
    lsf_reply = "LFD NOK\n";
    write_msg(client_fd, lsf_reply);
  }
  return;
}

void
delete_dir() {
  std::string dirname, bs_ip, bs_port;
  std::string status_reply, protocol;
  std::string bs_reply;
  //WE NEED TO READ TRAILING " " FROM USER PROTOCOL MSG
  read_msg(client_fd, 1);
  dirname = read_string(client_fd);
  if(find_user_dir(dirname, active_user, bs_ip, bs_port)) {

    get_backup_server_udp(bs_udp_fd, bs_ip, bs_port,
        bs_udp_addr, bs_addrlen);
    protocol = "DLB " + active_user + " " + dirname + "\n";
  
    if(sendto(bs_udp_fd, protocol.c_str(), protocol.size(), 0,
        (struct sockaddr*) &bs_udp_addr,
        bs_addrlen) == -1) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
    bs_reply = recvfrom_with_timeout(bs_udp_fd, bs_udp_addr, bs_addrlen, 10);
    close(bs_udp_fd);

    if(bs_reply.compare(0, 3,"DBR") == 0) {
      if(bs_reply.compare(4,2, "OK") == 0) {
        delete_user_dir_from_file(active_user, dirname);
        status_reply = "DDR OK\n";
        write_msg(client_fd, status_reply);
        return;
      }
      if(bs_reply.compare(4, 3, "NOK") == 0) {
        status_reply = "DDR NOK\n";
        write_msg(client_fd, status_reply);
        return;
      }
    } else {
      status_reply = "DDR ERR\n";
      write_msg(client_fd, status_reply);
      return;
    }

  }
  /* dir doesnt exist */
  std::cout << "Directory does not exist.\n";
  status_reply = "DDR NOK\n";
  write_msg(client_fd, status_reply);
  close(client_fd);
  return;
}

int
main(int argc, char **argv) {
  int pid, clientpid;
  std::string protocol;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

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
          protocol.clear(); protocol = read_msg(client_fd, 3);
          if(protocol.compare("DLU") == 0) {
            delete_user();
          } else if(protocol.compare("BCK") == 0) {
            backup_user_dir();
          } else if(protocol.compare("RST") == 0) {
            restore_user_dir();
          } else if(protocol.compare("LSD") == 0) {
            dir_list();
          } else if(protocol.compare("LSF") == 0) {
            file_list();
          } else if(protocol.compare("DEL") == 0) {
            delete_dir();
          }
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
      if(recvfrom(cs_udp_fd, buffer, sizeof(buffer), 0,
          (struct sockaddr*)&bs_udp_client_addr,
          &bs_client_addr_len) == -1) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
      }

      if(strncmp(buffer, "REG", 3) == 0) {
        register_backup_server(cs_udp_fd, bs_udp_client_addr,
            bs_client_addr_len, buffer);
        memset(buffer, '\0', 128);
      }else if(strncmp(buffer, "UNR", 3) == 0) {
        unregister_backup_server(cs_udp_fd, bs_udp_client_addr,
            bs_client_addr_len, buffer);
        memset(buffer, '\0', 128);
      }
    }
    close(cs_udp_fd);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_FAILURE);
}
