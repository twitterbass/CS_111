// NAME: John Tran
// EMAIL: johntran627@gmail.com
// ID: 005183495
// SLIPDAYS: 3


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


void causeSegFault() 
{
  char* ptr = NULL;
  *ptr = 'x';
}


void segfault_handler(int signum) 
{
  if (signum == SIGSEGV) {
    fprintf(stderr, "Caught & handled segmentation fault\n");
    exit(4);
  }
}


int main (int argc, char* argv[])
{
  int c;
  int option_index = 0;

  static struct option long_options[] = {
    { "input", required_argument, 0, 'i' },
    { "output", required_argument, 0, 'o' },
    { "segfault", no_argument, 0, 's' },
    { "catch", no_argument, 0, 'c' },
    { 0, 0, 0, 0 }
  };

  char* input_flag = NULL;
  char* output_flag = NULL;
  int segfault_flag = 0;
  

  while (1) {
    c = getopt_long(argc, argv, "iosc", long_options, &option_index);
    if (c == -1) {
      break;
    }

    switch(c) {
    case 'i':
      input_flag = optarg;
      break;
      
    case 'o':
      output_flag = optarg;
      break;

    case 's':
      segfault_flag = 1;
      break;

    case 'c':
      signal(SIGSEGV, segfault_handler);
      break;

    default:
      fprintf(stderr, "Valid options are: --input, --output, --segfault, --catch\n");
      exit(1);
    }
  }

  if (input_flag) {
    int ifd = open(input_flag, O_RDONLY);

    if (ifd >= 0) {
      close(0);
      dup(ifd);
      close(ifd);
    }
    else {
      close(ifd);
      fprintf(stderr, "Cannot open --input file: %s\n", optarg);
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(2);
    }
  }

  if (output_flag) {
    int ofd = creat(output_flag, 0644);
 
    if (ofd >= 0) {
      close(1);
      dup(ofd);
      close(ofd);
    }
    else {
      close(ofd);
      fprintf(stderr, "Cannot create/access --outfile: %s\n", optarg);
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(3);
    }    
  }

  if (segfault_flag) {
    causeSegFault();
  }

 

  char* buf;
  buf = (char*) malloc(sizeof(char));

  ssize_t read_stat = read(0, buf, sizeof(char));
  while (read_stat > 0) {
    ssize_t write_stat = write(1, buf, sizeof(char));
    if (write_stat < 0) {
      fprintf(stderr, "Cannot write to stdout: %s\n", strerror(errno));
      exit(-1);
    }

    read_stat = read(0, buf, sizeof(char));
    if (read_stat < 0) {
      fprintf(stderr, "Cannot read from stdin: %s\n", strerror(errno));
      exit(-1);
    }
  }

  if (read_stat < 0) {
    fprintf(stderr, "Cannot read from stdin: %s\n", strerror(errno));
    exit(-1);
  }
    

  free(buf);
  exit(0);
}

    
      

    
