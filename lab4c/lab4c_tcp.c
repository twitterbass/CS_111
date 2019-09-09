// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495


#include <mraa.h>
#include <mraa/aio.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>

int period = 1;
char scale = 'F';

int opt_log = 0;
int log_fd;

struct pollfd pfd;

mraa_aio_context sensor;

int is_time_to_report = 0;

char command[128];
int ptr = 0;

time_t next = 0;
struct tm *current;
struct timeval clk;

int stop_flag = 0;

char *id;
char *hostname;

int port;
int sfd;

void period_handler() {
	period = atoi(optarg);
	if (period < 1) {
		fprintf(stderr, "Period cannot be less than 1.\n");
		exit(1);
	}
}

void scale_handler() {
	scale = optarg[0];
	if (scale == 'F' || scale == 'C') {
		fprintf(stderr, "Invalid argument for scale option.\n");
		exit(1);
	}
}

void log_handler() {
	opt_log = 1;
	log_fd = creat(optarg, 0644);
	if (log_fd == -1) {
		fprintf(stderr, "creat() failed - %s\n", strerror(errno));
		exit(2);
	}
}

double get_temperature() {
	double reading = mraa_aio_read(sensor);

	int B = 4275;
	int R0 = 100000;
	double R = 1023 / reading - 1.0;
	R *= R0;

	double C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	double F = (C * 9) / 5 + 32;

	if (scale == 'C')
		return C;
	else
		return F;
}


void id_handler() {
	id = optarg;
	if (strlen(id) != 9) {
		fprintf(stderr, "ID must be 9 digits.\n");
		exit(1);
	}
}

void report_handler() {
	double temp = get_temperature();
	current = localtime(&(clk.tv_sec));
	
	dprintf(sfd, "%02d:%02d:%02d %.1f\n", current->tm_hour, 
		current->tm_min, current->tm_sec, temp);

	if (opt_log) {
		dprintf(log_fd, "%02d:%02d:%02d %.1f\n", current->tm_hour, 
			current->tm_min, current->tm_sec, temp);
	}
}

void shutdown_handler() {
	current = localtime(&(clk.tv_sec));

	dprintf(sfd, "%02d:%02d:%02d SHUTDOWN\n", current->tm_hour, 
		current->tm_min, current->tm_sec);

	if (opt_log) {
		dprintf(log_fd, "%02d:%02d:%02d SHUTDOWN\n", current->tm_hour, 
			current->tm_min, current->tm_sec);
	}
}

int compare_string(char *str) {
	return strcmp(command, str) == 0;
}

int compare_string_with_diff(char *str) {
	return strncmp(command, str, strlen(str)) == 0;
}

void process_command() {

	if (opt_log) dprintf(log_fd, "%s\n", command);

	if (compare_string("SCALE=F")) {
		scale = 'F';
	}
	else if (compare_string("SCALE=C")) {
		scale = 'C';
	}
	else if (compare_string_with_diff("PERIOD=")) {
		char *seconds = command + strlen("PERIOD=");
		period = atoi(seconds);
	}
	else if (compare_string("STOP")) {
		stop_flag = 1;
	}
	else if (compare_string("START")) {
		stop_flag = 0;
	}
	else if (compare_string("LOG ")) {
		// do nothing
	}
	else if (compare_string("OFF")) {
		shutdown_handler();
		exit(0);
	}

	memset(command, 0, 128);
	ptr = 0;
}

void extract_commands(char *buf, int size) {
	int i;
	for (i = 0; i < size; i++) {
		if (buf[i] == '\n')
			process_command();
		else
			command[ptr++] = buf[i];
	}
}

void polling() {
	char buf[128];

	if (poll(&pfd, 1, 0) == -1) {
		fprintf(stderr, "poll() failed - %s\n", strerror(errno));
		exit(2);
	}

	if (pfd.revents & POLLIN) {
		int ret = read(sfd, buf, 128);
		if (ret == -1) {
			fprintf(stderr, "read() failed - %s\n", strerror(errno));
			exit(1);
		}
		extract_commands(buf, ret);
	}

	if (pfd.revents & POLLHUP) exit(0);

	if (pfd.revents & POLLERR) {
		fprintf(stderr, "POLLERR detected - %s\n", strerror(errno));
		exit(1);
	}
}


void report_id() {
	dprintf(sfd, "ID=%s\n", id);
	if (opt_log) dprintf(log_fd, "ID=%s\n", id);
}

void initialize_socket() {
	struct hostent *server;
	struct sockaddr_in server_address;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1) {
		fprintf(stderr, "socket() failed - %s\n", strerror(errno));
		exit(1);
	}

	server = gethostbyname(hostname);

	memset((char*)&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	memcpy((char*)&server_address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
	server_address.sin_port = htons(port);

	if (connect(sfd, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
		fprintf(stderr, "connect() failed - %s\n", strerror(errno));
		exit(1);
	 }

	report_id();
}

void initialize() {
	sensor = mraa_aio_init(1);
	if (sensor == NULL) {
		fprintf(stderr, "Error initializing sensor\n");
		exit(1);
	}

	initialize_socket();
	
	pfd.fd = sfd;
	pfd.events = POLLIN | POLLHUP | POLLERR;
}

int main(int argc, char* argv[]) {
	int c;
	static struct option long_options[] = {
		{"period", required_argument, 0, 'p'},
		{"scale",  required_argument, 0, 's'},
		{"log",    required_argument, 0, 'l'},
		{"id",     required_argument, 0, 'i'},
		{"host",   required_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "", long_options, NULL);

		if (c == -1) break;

		switch (c) {
		case 'p':
			period_handler();
			break;
		case 's':
			scale_handler();
			break;
		case 'l':
			log_handler();
			break;
		case 'i':
			id_handler();
			break;
		case 'h':
			hostname = optarg;
			break;
		default:
			fprintf(stderr, "Unrecognized arguments.\n");
			exit(1);
		}
	}

	port = atoi(argv[argc - 1]);
	if (port <= 0) {
		fprintf(stderr, "Port number must be positive.\n");
		exit(1);
	}

	initialize();

	while (1) {
		gettimeofday(&clk, 0);

		if (!stop_flag && clk.tv_sec >= next) {
			report_handler();
			next = clk.tv_sec + period;
		}
		polling();
	}

	if (opt_log) close(log_fd);
	mraa_aio_close(sensor);

	exit(0);
}