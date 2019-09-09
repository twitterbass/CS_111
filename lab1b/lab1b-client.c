// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495
// SLIPDAYS: 5


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mcrypt.h>

struct termios current;

int port_flag = 0;
int encrypt_flag = 0;
int log_flag = 0;

MCRYPT encrypt;
MCRYPT decrypt;

int port_num = -1;
int log_fd = -1;
int encrypt_fd = -1;

struct stat keyStat;
int socket_fd;

char key_buf[256];

const char* HOST_NAME = "localhost";


void tcgetattr_handler(int fd, struct termios *termios_p) 
{
  int status = tcgetattr(fd, termios_p);
  if (status == -1) {
    fprintf(stderr, "tcgetattr() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void tcsetattr_handler(int fd, int optional_actions, struct termios *termios_p)
{
  int status = tcsetattr(fd, optional_actions, termios_p);
  if (status == -1) {
    fprintf(stderr, "tcsetattr() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void terminal_setup()
{
  tcgetattr_handler(STDIN_FILENO, &current);
  
  struct termios temp;
  tcgetattr(STDIN_FILENO, &temp);

  temp.c_iflag = ISTRIP;
  temp.c_oflag = 0;
  temp.c_lflag = 0;

  tcsetattr(STDIN_FILENO, TCSANOW, &temp);
}


ssize_t read_handler(int fd, char * buf, int size) 
{
  ssize_t status = read(fd, buf, size);
  if (status == -1) {
    fprintf(stderr, "read() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


ssize_t write_handler(int fd, char * buf, int size) 
{
  ssize_t status = write(fd, buf, size);
  if (status == -1) {
    fprintf(stderr, "write() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


int open_handler(char* pathname, int flags)
{
  int status = open(pathname, flags);
  if (status == -1) {
    fprintf(stderr, "open() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


int creat_handler(const char* pathname, mode_t mode)
{
  int status = creat(pathname, mode);
  if (status == -1) {
    fprintf(stderr, "creat() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


void close_handler(int fd)
{
  int status = close(fd);
  if (status == -1) {
    fprintf(stderr, "close() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void poll_handler(struct pollfd *fds, nfds_t nfds, int timeout)
{
  int status = poll(fds, nfds, timeout);
  if (status == -1) {
    fprintf(stderr, "poll() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void data_sent_log_handler(char* buf, ssize_t size)
{
  int buf_size;
  buf_size = (int) size;
  if (dprintf(log_fd, "SENT %d bytes: ", buf_size) == -1) {
    fprintf(stderr, "dprintf() failed - %s\n", strerror(errno));
    exit(1);
  }
  write_handler(log_fd, buf, size);
  write_handler(log_fd, "\n", 1);
}


void data_received_log_handler(char* buf, ssize_t size)
{
  int buf_size;
  buf_size = (int) size;
  if (dprintf(log_fd, "RECEIVED %d bytes: ", buf_size) == -1) {
    fprintf(stderr, "dprintf() failed - %s\n", strerror(errno));
    exit(1);
  }
  write_handler(log_fd, buf, size);
  write_handler(log_fd, "\n", 1);
}


int socket_handler(int domain, int type, int protocol)
{
  int status = socket(domain, type, protocol);
  if (status == -1) {
    fprintf(stderr, "socket() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


void connect_handler(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int status = connect(sockfd, addr, addrlen);
  if (status == -1) {
    fprintf(stderr, "connect() failed - %s\n", strerror(errno));
    exit(1);
  }
}


struct hostent* gethostbyname_handler(const char* name)
{
  struct hostent* result = gethostbyname(name);
  if (result == NULL) {
    fprintf(stderr, "gethostbyname() failed - %s\n", strerror(errno));
    exit(1);
  }
  return result;
}


int client_connect(const char* host_name, unsigned int port) 
{
  struct sockaddr_in serv_addr; // Encode the IP address & the port for the remote
  int sockfd = socket_handler(AF_INET, SOCK_STREAM, 0);

  struct hostent* server = gethostbyname_handler(host_name);
  
  // Convert host_name to IP addr
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  // Copy IP address from server to serv_Addr
  serv_addr.sin_port = htons(port); // Set up the port
  connect_handler(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

  return sockfd;
}


void terminal_write(int fd, char* buf, ssize_t io_status)
{
  ssize_t i;
  for (i = 0; i < io_status; i++) {
    char c = buf[i];
    if (c == '\r' || c == '\n')
      write_handler(fd, "\r\n", 2);
    else
      write_handler(fd, &c, 1);
  }
}


void keyboard_io_handler()
{
  char buf[256];
  ssize_t io_status = read_handler(0, buf, 256);

  terminal_write(1, buf, io_status);

  if (encrypt_flag) {
    mcrypt_generic(encrypt, buf, io_status);
  }

  if (log_flag) {
    data_sent_log_handler(buf, io_status);
  }
  
  write_handler(socket_fd, buf, io_status);
}


void socket_io_handler()
{
  char buf[256];
  ssize_t io_status = read_handler(socket_fd, buf, 256);

  if (io_status == 0) {
    close_handler(socket_fd);
    exit(0);
  }

  if (log_flag) {
    data_received_log_handler(buf, io_status);
  }

  if (encrypt_flag) { 
    mdecrypt_generic(decrypt, buf, io_status);
  }

  terminal_write(1, buf, io_status);
}


void remaining_io_handler()
{
  char buf[256];
  ssize_t io_status;

  while ( (io_status =  read_handler(socket_fd, buf, 256)) != 0 ) {
    if (log_flag) {
      data_received_log_handler(buf, io_status);
    }

    if (encrypt_flag) { 
      mdecrypt_generic(decrypt, buf, io_status);
    }

    terminal_write(1, buf, io_status);
  }
  if (io_status == 0) {
    close(socket_fd);
    exit(0);
  }
}


void port_io_handler()
{
  struct pollfd pollfds[2];

  pollfds[0].fd = 0;
  pollfds[0].events = POLLIN + POLLHUP + POLLERR;

  pollfds[1].fd = socket_fd;
  pollfds[1].events = POLLIN + POLLHUP + POLLERR;

  while (1) {
    poll_handler(pollfds, 2, -1); // -1 means no time out

    // Read from the keyboard, echo it to stdout, and forward it to the shell
    if (pollfds[0].revents & POLLIN) {
      keyboard_io_handler();
    }
    
    // Read from the shell pipe, and write it to stdout
    if (pollfds[1].revents & POLLIN) {
      socket_io_handler();   
    }

    if (pollfds[0].revents & (POLLHUP | POLLERR)) {
      remaining_io_handler();
    }

    if (pollfds[1].revents & (POLLHUP | POLLERR)) {
      close_handler(socket_fd);
      exit(0);
    }
  }
}
      

MCRYPT init_session()
{
  MCRYPT session = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  
  char *iv = malloc(mcrypt_enc_get_iv_size(session));

  memset(iv, 0, mcrypt_enc_get_iv_size(session));
  
  int key_length = mcrypt_enc_get_key_size(session);

  mcrypt_generic_init(session, key_buf, key_length, iv);
  
  return session;
}


void close_session(MCRYPT session) 
{
  mcrypt_generic_deinit(session);
  mcrypt_module_close(session);
}


void restore_current_mode() 
{
  tcsetattr_handler(STDIN_FILENO, TCSANOW, &current);

  if (encrypt_flag) {
    close_session(encrypt);
    close_session(decrypt);
  }
}


FILE* fopen_handler(char* pathname, char* mode)
{
  FILE* status = fopen(pathname, mode);
  if (status == NULL) {
    fprintf(stderr, "fopen() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


int fread_handler(char* ptr, size_t size, size_t nmemb, FILE* stream)
{
  int status = fread(ptr, size, nmemb, stream);
  if (status != (sizeof(char) * 256)) {
    fprintf(stderr, "fread() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


void fclose_handler(FILE* stream) 
{
  int status = fclose(stream);
  if (status != 0) {
    fprintf(stderr, "fclose() failed - %s\n", strerror(errno));
    exit(1);
  }
}


int main(int argc, char* argv[])
{
  static struct option long_options[] = {
    { "port", required_argument, 0, 'p' },
    { "encrypt", required_argument, 0, 'e' },
    { "log", required_argument, 0, 'l' },
    { 0, 0, 0, 0 }
  };

  int c;

  while (1) {
    c = getopt_long(argc, argv, "", long_options, NULL);
    
    if (c == -1) break;

    switch (c) {
    case 'p':
      port_flag = 1;
      port_num = atoi(optarg);
      break;

    case 'l':
      log_flag = 1;
      log_fd = creat_handler(optarg, 0644);
      break;

    case 'e':
      encrypt_flag = 1;
      FILE* key_file = fopen_handler(optarg, "r");
      fread(key_buf, sizeof(char), 256, key_file);
      fclose_handler(key_file);
      break;
      
    default:
      fprintf(stderr, "Valid options are:  --port=portNum, --encrypt=fileName, --log=fileName\n");
      exit(1);
    }
  }
  
  terminal_setup();
  atexit(restore_current_mode);

  socket_fd = client_connect(HOST_NAME, port_num);

  if (encrypt_flag) { 
    encrypt = init_session();
    decrypt = init_session();
  }

  port_io_handler();
  
  exit(0);
}
