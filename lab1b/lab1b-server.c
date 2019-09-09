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
#include <arpa/inet.h>

#include <mcrypt.h>


pid_t child_pid;

MCRYPT encrypt;
MCRYPT decrypt;


int port_flag = 0;
int encrypt_flag = 0;

int port_num = -1;
int encrypt_fd = -1;

int new_socket_fd;

struct stat keyStat;

char key_buf[256];


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


void pipe_handler(int pipefd[2]) 
{
  int status = pipe(pipefd);
  if (status == -1) {
    fprintf(stderr, "pipe() failed - %s\n", strerror(errno));
    exit(1);
  }
}


pid_t fork_handler()
{ 
  pid_t status = fork();
  if (status == -1) {
    fprintf(stderr, "fork() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}


void dup2_handler(int oldfd, int newfd)
{
  int status = dup2(oldfd, newfd);
  if (status == -1) {
    fprintf(stderr, "dup() failed - %s\n", strerror(errno));
    exit(1);
  }
}


int creat_handler(char* pathname, mode_t mode)
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


void sigpipe_handler(int signum)
{ 
  fprintf(stderr, "Received a SIGPIPE - %d\n", signum);
  exit(0);
}


void child_process_handler(int to_shell[], int from_shell[])
{
  close_handler(to_shell[1]);
  close_handler(STDIN_FILENO);
  dup2_handler(to_shell[0], STDIN_FILENO);  
  close_handler(to_shell[0]);

  close_handler(from_shell[0]);
  close_handler(STDOUT_FILENO);
  dup2_handler(from_shell[1], STDOUT_FILENO);
  close_handler(STDERR_FILENO);
  dup2_handler(from_shell[1], STDERR_FILENO);
  close_handler(from_shell[1]);

  char* fileName = "/bin/bash";
  char* argv = fileName;

  int status = execlp(fileName, argv, NULL); 
  if (status == -1 ) {
    fprintf(stderr, "execlp() failed - %s\n", strerror(errno));
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


void waitpid_handler(pid_t pid, int* status, int options)
{
  pid_t stat = waitpid(pid, status, options);
  if (stat == -1) {
    fprintf(stderr, "waitpid() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void kill_handler(pid_t pid, int sig)
{
  int status = kill(pid, sig);
  if (status == -1) {
    fprintf(stderr, "kill() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void pipe_write(int to_shell[], char* buf, ssize_t io_status)
{
  ssize_t i;
  for (i = 0; i < io_status; i++) {
    char c = buf[i];
    if (c == '\004') {
      close_handler(to_shell[1]);
      continue;
    }

    if (c == '\003') {
      kill_handler(child_pid, SIGINT);
      continue;
    }

    if (c == '\r' || c == '\n') {
      write_handler(to_shell[1], "\n", 1);
      continue;
    }

    write_handler(to_shell[1], &c, 1);
  }
}


void socket_io_handler(int to_shell[])
{
  char buf[256];
  ssize_t io_status = read_handler(new_socket_fd, buf, 256);
  if (io_status == 0) close_handler(to_shell[1]);

  if (encrypt_flag) {
    mdecrypt_generic(decrypt, buf, io_status);
  }

  pipe_write(to_shell, buf, io_status);
}


void shell_pipe_io_handler(int from_shell[])
{
  char buf[256];
  ssize_t io_status = read_handler(from_shell[0], buf, sizeof(buf));
  

  if (io_status == 0) {
    close_handler(new_socket_fd);
    close(from_shell[0]);
    exit(0);
  }

  if (encrypt_flag) { 
    mcrypt_generic(encrypt, buf, io_status);
  }

  write_handler(new_socket_fd, buf, io_status);
}

void remaining_io_handler(int from_shell[])
{
  char buf[256];
  ssize_t io_status;

  while ( (io_status =  read_handler(from_shell[0], buf, sizeof(buf))) != 0 ) {
    if (encrypt_flag) { 
      mcrypt_generic(encrypt,buf, io_status);
    }
    
    write_handler(new_socket_fd, buf, io_status);
  } 
    close(from_shell[0]);
    close(new_socket_fd);
    exit(0);
}
    

void parent_process_handler(int to_shell[], int from_shell[])
{
  close_handler(to_shell[0]);
  close_handler(from_shell[1]);

  if ( signal(SIGPIPE, sigpipe_handler) == SIG_ERR ) {
    fprintf(stderr, "signal() failed - %s\n", strerror(errno));
    exit(1);
  }    

  struct pollfd pollfds[2];

  // POLLIN: data to read
  // POLLHUP: FD is closed by the otherside
  // POLLERR: error occurred
  pollfds[0].fd = new_socket_fd;
  pollfds[0].events = POLLIN + POLLHUP + POLLERR;

  pollfds[1].fd = from_shell[0];
  pollfds[1].events = POLLIN + POLLHUP + POLLERR;

  while (1) {
    poll_handler(pollfds, 2, -1); // -1 means no time out

    // Read from the keyboard, echo it to stdout, and forward it to the shell
    if (pollfds[0].revents & POLLIN) {
      socket_io_handler(to_shell);
    }
    
    // Read from the shell pipe, and write it to stdout
    if (pollfds[1].revents & POLLIN) {
      shell_pipe_io_handler(from_shell);   
    }
    
    if (pollfds[0].revents & (POLLHUP | POLLERR)) {
      remaining_io_handler(from_shell);
      return;
    }

    if (pollfds[1].revents & (POLLHUP | POLLERR)) {
      close(to_shell[1]);
      close(new_socket_fd);
      exit(0);
    } 
  }
}
  

void shell_write() 
{
  int to_shell[2];
  pipe_handler(to_shell);

  int from_shell[2];
  pipe_handler(from_shell);

 
  child_pid = fork_handler();

  if (child_pid == 0) { // Child
    child_process_handler(to_shell, from_shell);    
  }
  else { // Parent
    parent_process_handler(to_shell, from_shell);
  }
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


void bind_handler(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
  int status = bind(sockfd, addr, addrlen);
  if (status == -1) {
    fprintf(stderr, "bind() failed - %s\n", strerror(errno));
    exit(1);
  }
}


void listen_handler(int sockfd, int backlog) 
{
  int status = listen(sockfd, backlog);
  if (status == -1) {
    fprintf(stderr, "listen() failed - %s\n", strerror(errno));
    exit(1);
  }
}


int accept_handler(int sockfd, struct sockaddr *addr, socklen_t *addrlen) 
{
  int status = accept(sockfd, addr, addrlen);
  if (status == -1) {
    fprintf(stderr, "accept() failed - %s\n", strerror(errno));
    exit(1);
  }

  return status;
}


int server_connect(unsigned int port)
// port_num: which port server listens to, return socket for subsequent communication
{
  struct sockaddr_in serv_addr, cli_addr;

  unsigned int cli_len = sizeof(struct sockaddr_in);
  int listen_fd = socket_handler(AF_INET, SOCK_STREAM, 0);

  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY; // Server can accept connection from any client IP
  serv_addr.sin_port = htons(port);

  bind_handler(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  listen_handler(listen_fd, 5);

  new_socket_fd = accept_handler(listen_fd, (struct sockaddr *) &cli_addr, &cli_len);

  return new_socket_fd;
}

MCRYPT init_session()
{  
  MCRYPT session = mcrypt_module_open("twofish", NULL, "cfb", NULL);

  char* iv = malloc(mcrypt_enc_get_iv_size(session));

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
 

void child_process_exit_handler()
{
  int status = 0;
  waitpid_handler(child_pid, &status, 0);
  
  if (encrypt_flag) {
    close_session(encrypt);
    close_session(decrypt);
  }
 
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status),
	  WEXITSTATUS(status));
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

    case 'e':
      encrypt_flag = 1;
      
      FILE* key_file = fopen_handler(optarg, "r");
      fread(key_buf, sizeof(char), 256, key_file);
      fclose_handler(key_file);
      break;
     
    default:
      fprintf(stderr, "Valid options are: --port=portNum, --encrypt=fileName\n");
      exit(1);
    }
  }

  if (encrypt_flag) {
    encrypt = init_session();
    decrypt = init_session();
  }

  atexit(child_process_exit_handler);
  
  new_socket_fd = server_connect(port_num);
  
  shell_write();
  
  exit(0);
}
