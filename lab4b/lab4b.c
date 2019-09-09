// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495
// SLIPDAYS: 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <poll.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>


int period_flag = 1;
char scale_flag = 'F';
int log_flag = 0;
int log_fd;

int ptr = 0;

int stop_flag = 0;

struct pollfd pfd;

char command[128];

mraa_aio_context tempSensor;
mraa_gpio_context button;


struct timeval clk;
struct tm *current;
time_t next = 0;


void shut_down_handler() {
  current = localtime(&(clk.tv_sec));

  if (log_flag) {
    dprintf(log_fd, "%02d:%02d:%02d SHUTDOWN\n", current->tm_hour,
	    current->tm_min, current->tm_sec);
  }
  else {
    fprintf(stdout, "%02d:%02d:%02d SHUTDOWN\n", current->tm_hour,
	    current->tm_min, current->tm_sec);
  }
  exit(0);
}


void log_command(char* cmd) {
  if (log_flag) {
    dprintf(log_fd, "%s\n", cmd); 
  }
  
}





void invalid_argument(const char* optarg) {
  fprintf(stderr, "Invalid argument for this option: %s\n", optarg);
  exit(1);
}

void period_option_handler() {
  period_flag = atoi(optarg);
  if (period_flag <= 0) {
    invalid_argument(optarg);
  }
}

void scale_option_handler() {
  char c = optarg[0];
  if ((strlen(optarg) != 1) || (c != 'C' && c != 'F')) {
    invalid_argument(optarg);
  }

  scale_flag = c;
}

int creat_handler(const char* pathname, mode_t mode) {
  int status = creat(pathname, mode);
  if (status == -1) {
    fprintf(stderr, "creat() failed - %s\n", strerror(errno));
    exit(1);
  }

  return status;
}

void log_option_handler() {
  log_flag = 1;
  log_fd = creat(optarg, 0644);
}

void poll_handler(struct pollfd *fds, nfds_t nfds, int timeout) {
  if (poll(fds, nfds, timeout) == -1) {
    fprintf(stderr, "poll() failed - %s\n", strerror(errno));
    exit(1);
  }
}

double get_temperature() {
  int B = 4275;
  int R0 = 100000;

  int reading = mraa_aio_read(tempSensor);
  double R = 1023.0 / ((double)reading) - 1.0;
  R = R0 * R;
  double C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  double F = (C * 9)/5 + 32;
  if (scale_flag == 'C')
    return C;
  else
    return F;
}
int compare_string(char *str) {
  return (strcmp(command, str) == 0);   
}

int compare_string_with_diff_length(char *str) {
  return (strncmp(command, str, strlen(str)) == 0);
}

void command_line_handler() {
  if (compare_string("SCALE=F")) {
    scale_flag = 'F';
    log_command("SCALE=F");
  }
  else if (compare_string("SCALE=C")) {
    scale_flag = 'C';
    log_command("SCALE=C");
  }
  else if (compare_string("STOP")) {
    stop_flag = 1;
    log_command("STOP");
  }
  else if (compare_string("START")) {
    stop_flag = 0;
    log_command("START");
  }
  else if (compare_string("OFF")) {
    
    log_command("OFF");
    shut_down_handler();
  }
  else if (compare_string_with_diff_length("LOG")) {
    log_command(command);
  }
  else if (compare_string_with_diff_length("PERIOD=")) {
    log_command(command);
    
    char *t = command + strlen("PERIOD=");
    period_flag = atoi(t);
  }

	memset(command, 0, 128);
	ptr = 0;

}

void extract_commands(char *buf, int size) {
	int i;

	for (i = 0; i < size; i++) {
		if (buf[i] == '\n') 
			command_line_handler();
		else
			command[ptr++] = buf[i];
	}
}

void polling() {
  char buf[128];
  poll(&pfd, 1, 0);

  if (pfd.revents & POLLIN) {
    int io_status = read(0, buf, 128);
	extract_commands(buf, io_status);
	//fgets(command, 128, stdin);
    //command_line_handler();
  }
  if (pfd.revents & POLLHUP) 
    exit(0);
  if (pfd.revents & POLLERR) {
    fprintf(stderr, "POLLERR caught - %s\n", strerror(errno));
    exit(1);
  }
}

void report_handler() {
  double temp = get_temperature();
  current = localtime(&(clk.tv_sec));

  if (log_flag) {
    dprintf(log_fd, "%02d:%02d:%02d %.1f\n",current->tm_hour,
	    current->tm_min, current->tm_sec, temp);
  }
  else {
    fprintf(stdout, "%02d:%02d:%02d %.1f\n", current->tm_hour,
	    current->tm_min, current->tm_sec, temp);
  }
}


int main(int argc, char* argv[]) {

  int c;
  static struct option long_options[] = {
    { "period", required_argument, 0, 'p' },
    { "scale", required_argument, 0, 's' },
    { "log", required_argument, 0, 'l' },
    { 0, 0, 0, 0 }
  };

  while (1) {
    c = getopt_long(argc, argv, "", long_options, NULL);
    if (c == -1) break;

    switch(c) {
    case 'p':
      period_option_handler();
      break;
    case 's':
      scale_option_handler();
      break;
    case 'l':
      log_option_handler();
      break;
    default:
      fprintf(stderr, "Valid options are: --period, --scale, --log\n");
      exit(1);
    }
  }

  // Initialize
  tempSensor = mraa_aio_init(1);
  button = mraa_gpio_init(60);
  mraa_gpio_dir(button, MRAA_GPIO_IN);

  pfd.fd = 0;
  pfd.events = POLLIN | POLLHUP | POLLERR;

 

  while (1) {

    gettimeofday(&clk, 0);


    if (!stop_flag && clk.tv_sec >= next) {
      report_handler();
      next = clk.tv_sec + period_flag;
    }

    if (mraa_gpio_read(button)) 
      shut_down_handler();

    polling();
  }

  // Clean up
  if (log_flag) 
    close(log_fd);
  mraa_aio_close(tempSensor);
  mraa_gpio_close(button);

  exit(0);
}
