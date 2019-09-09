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


struct termios current;
pid_t child_pid;


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


void restore_current_mode(void) 
{
  tcsetattr_handler(STDIN_FILENO, TCSANOW, &current);
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


void terminal_write() 
{
  char buf[256];
  int endOfFile = 0;
  while(1) {
    ssize_t status = read_handler(STDIN_FILENO, buf, sizeof(buf));

    int i;
    for (i = 0; i < status; i++) {
      char c = buf[i];
      if (c == '\004') {
	endOfFile = 1;
	break;
      }
      else if (c == '\r' || c == '\n') {
	write_handler(STDOUT_FILENO, "\r\n", 2);
	continue;
      }
      else {
	write_handler(STDOUT_FILENO, &c, 1);
      }
    }

    if (endOfFile) break;
  }
}

/////////////////////////////////////////////////////////////

//                       PART B

///////////////////////////////////////////////////////////////


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

void child_process_exit_handler()
{
  int status = 0;
  waitpid_handler(child_pid, &status, 0);

  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status),
	  WEXITSTATUS(status));
}


void keyboard_io_handler(int to_shell[])
{
  char buf[256];
  ssize_t io_status = read_handler(STDIN_FILENO, buf, sizeof(buf));

  ssize_t i;
  for (i = 0; i < io_status; i++) {
    char c = buf[i];
    if (c == '\004') {
      write_handler(STDOUT_FILENO, "^D", 2);
      close(to_shell[1]);
      continue;
    }

    if (c == '\003') {
      write_handler(STDOUT_FILENO, "^C", 2);
      kill_handler(child_pid, SIGINT);
      continue;
      
    }

    if (c == '\n' || c == '\r') {
      write_handler(STDOUT_FILENO, "\r\n", 2);
      write_handler(to_shell[1], "\n", 1);
      continue;
    }

    write_handler(STDOUT_FILENO, &c, 1);
    write_handler(to_shell[1], &c, 1);
  }   
}

void shell_pipe_io_handler(int from_shell[])
{
  char buf[256];
  ssize_t io_status = read_handler(from_shell[0], buf, sizeof(buf));

  ssize_t i;
  for (i = 0; i < io_status; i++) {
    char c = buf[i];
    if (c == '\n') {
      write_handler(STDOUT_FILENO, "\r\n", 2);
      continue;
    }

    write_handler(STDOUT_FILENO, &c, 1);
  }
}

void remaining_io_handler(int from_shell[])
{
  char buf[256];
  ssize_t io_status;

  while ( (io_status =  read_handler(from_shell[0], buf, sizeof(buf))) != 0 ) {
    ssize_t i;
    for(i = 0; i < io_status; i++) {
      char c = buf[i];
      if (c == '\n') {
	write_handler(STDOUT_FILENO, "\r\n", 2);
	continue;
      }
      write_handler(STDOUT_FILENO, &c, 1);
    }
  }
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
  pollfds[0].fd = 0;
  pollfds[0].events = POLLIN + POLLHUP + POLLERR;

  pollfds[1].fd = from_shell[0];
  pollfds[1].events = POLLIN + POLLHUP + POLLERR;

  while (1) {
    poll_handler(pollfds, 2, -1); // -1 means no time out

    // Read from the keyboard, echo it to stdout, and forward it to the shell
    if (pollfds[0].revents & POLLIN) {
      keyboard_io_handler(to_shell);
    }
    
    // Read from the shell pipe, and write it to stdout
    if (pollfds[1].revents & POLLIN) {
      shell_pipe_io_handler(from_shell);   
    }

    if (pollfds[0].revents & POLLHUP) {
      remaining_io_handler(from_shell);
      return;
    }

    if (pollfds[0].revents & POLLERR) {
      fprintf(stderr, "Caught stdin POLLERR - %s\n", strerror(errno));
      return;
    }

    if (pollfds[1].revents & POLLHUP) {
      remaining_io_handler(from_shell);
      return;
    }

    if (pollfds[1].revents & POLLERR) {
      fprintf(stderr, "Caught shell pipe POLLERR - %s\n", strerror(errno));
      return;
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


int main(int argc, char* argv[])
{
  static struct option long_options[] = {
    { "shell", required_argument, 0, 's' },
    { 0, 0, 0, 0 }
  };

  int c;
  int shell_flag = 0;

  while (1) {
    c = getopt_long(argc, argv, "", long_options, NULL);
    
    if (c == -1) break;

    switch (c) {
    case 's':
      shell_flag = 1;
      break;
      
    default:
      fprintf(stderr, "Valid option is --shell, or no option at all\n");
      exit(1);
    }
  }
  
  terminal_setup();
  atexit(restore_current_mode);
 
  if (shell_flag) {    
    shell_write();
    atexit(child_process_exit_handler);
    exit(0);    
  }

  terminal_write();
  exit(0);
}
