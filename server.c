#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define log(...) do{time_t timer;char _log_t_buf[26]; struct tm* tm_info;\
	time(&timer); tm_info = localtime(&timer);\
	strftime(_log_t_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);\
	printf("%s -- ", _log_t_buf);\
	printf(__VA_ARGS__);putchar('\n');}while(0)

#define die(...) do{int errsave=errno;log(__VA_ARGS__);perror("Error");exit(1);}while(0)

#define BUFSIZE 8096

void web(int fd) {
	int ret;
	static char buf[BUFSIZE+1];
	static char output_buf[BUFSIZE+1];

	ret = read(fd, buf, BUFSIZE);
	if (ret == 0 || ret == -1) {
		die("Failed to read request");
	}

	if (ret > 0 && ret < BUFSIZE) {
		// Add null terminator if we read a valid size
		buf[ret] = '\0';
	}
	else {
		// Not sure what this is about.
		buf[0] = '\0';
	}

	// Dispose of line breaks and such
	for (int i = 0; i < ret; i++) {
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = '*';
		}
	}

	log("Request: %s", buf);

	//snprintf(buf, BUFSIZE, "Hello, World!");
	//(void) write(fd, buf, )
	dprintf(fd, "HTTP/1.1 200 OK\nServer: spence/1.0\nContent-Length: 14\nConnection: close\nContent-Type: text/html\n\n");
	dprintf(fd, "Hello, World!");

	close(fd);
	return;
}

int serve(int port) {
	int listenfd, socketfd, pid;
	static struct sockaddr_in serv_addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		die("Failed to open socket");
	}

	serv_addr = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_addr = {
			.s_addr = htonl(INADDR_ANY),
		},
		.sin_port = htons(port),
	};

	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		die("Failed to bind socket to port");
	}

	if (listen(listenfd, 64) < 0) {
		die("Failed to listen to socket");
	}

	for (;;) {
		if ((socketfd = accept(listenfd, NULL, NULL)) < 0) {
			log("Error accepting socket connection");
			perror("accept");
			die("listenfd is %d", listenfd);
		}
		if ((pid = fork()) < 0) {
			log("Could not fork to handle request");
		}
		else {
			if (pid == 0) {
				// Child handles request
				(void) close(listenfd);
				web(socketfd);
				exit(EXIT_SUCCESS);
			}
			else {
				// Parent closes and moves to next
				(void) close(socketfd);
			}
		}
	}
	
	return 0;
}
