#include "slughttpd.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void not_found(Request req, Response resp);

typedef void(*Acceptor)(Server s, int listenfd);
void accept_fork(Server s, int listenfd);
void accept_nofork(Server s, int listenfd);

typedef struct ResponseObj {
	char *buf;
	size_t bufi;
	int status;
} ResponseObj;

char *resp_buf(Response r) {
	return r->buf + r->bufi;
}

int resp_nleft(Response r) {
	return BUFSIZE - r->bufi;
}

void resp_wrote_n(Response r, size_t wrote) {
	r->bufi += wrote;
}

void resp_status(Response r, int n) {
	r->status = n;
}

typedef struct RequestObj {
	char *buf;
} RequestObj;

String req_path(Request r) {
	int i, len;
	for (i = 0; r->buf[i] != '/'; i++);
	// i now points to the first leading slash.
	for (len = 1; r->buf[i+len] != ' '; len++);
	// i points to the first leading slash, and len points to the last character
	// before the next space.
	return (String) {.chars = r->buf+i, .len = len};
}

String req_post_arg(Request r, String arg_name) {
	// Painfully and inefficiently finds the argument specified.
	int i = BUFSIZE;
	for (int i = BUFSIZE; r->buf[i] != '*'; i--) {
		if ((r->buf[i-1] == '*' || r->buf[i-1] == '&')
				&& strncmp(r->buf+i, arg_name.chars, arg_name.len) == 0) {
			// Found it.
			// If we have "abcd=1234", i points to the 'a'. Skip the arg chars
			// and equals sign.
			i += 1+arg_name.len;
			int len;
			for (len = 1; r->buf[i+len-1] != '&'
					&& r->buf[i+len-1] != '*'
					&& r->buf[i+len-1] != '\0';
					len++);
			return (String) {.chars = r->buf+i, .len = len};
		}
	}
	return (String) { .chars = NULL, .len = 0, };
}

typedef struct ServerObj {
	int port, nhandlers;
	struct handler_info {
		const char *path;
		Handler h;
	} *handlers;
	Acceptor acceptor;
} ServerObj;

Server new_server(int port, bool multiprocess) {
	Server s = calloc(1, sizeof(ServerObj));
	s->port = port;
	s->acceptor = multiprocess ? accept_fork : accept_nofork;
	return s;
}

void accept_and_handle(Server s, int fd) {
	s->acceptor(s, fd);
}

int handle_path(Server s, const char *path, Handler h) {
	s->nhandlers += 1;
	s->handlers = realloc(s->handlers, s->nhandlers);
	if (s->handlers == NULL) {
		return -1;
	}
	s->handlers[s->nhandlers-1].path = path;
	s->handlers[s->nhandlers-1].h = h;
	return 0;
}

Handler get_handler(Server s, String path) {
	// What performance? Did someone say trie? I didn't think so.
	for (int i = 0; i < s->nhandlers; i++) {
		if (strncmp(path.chars, s->handlers[i].path, path.len) == 0) {
			return s->handlers[i].h;
		}
	}
	return NULL;
}

void handle_thread(Server s, int fd) {
	int ret;
	static char req_buf[BUFSIZE+1];
	static char resp_buf[BUFSIZE+1];
	static RequestObj req = {
		.buf = req_buf,
	};
	static ResponseObj resp = {
		.buf = resp_buf,
		.bufi = 0,
		.status = 200,
	};

	// Clear the response object for the nofork case
	resp.bufi = 0;
	resp.status = 200;

	ret = read(fd, req.buf, BUFSIZE);
	if (ret == 0 || ret == -1) {
		die("Failed to read request");
	}

	if (ret > 0 && ret < BUFSIZE) {
		// Add null terminator if we read a valid size
		req.buf[ret] = '\0';
	}
	else {
		// Not sure what this is about.
		req.buf[0] = '\0';
	}

	// Dispose of line breaks and such
	for (int i = 0; i < ret; i++) {
		if (req.buf[i] == '\r' || req.buf[i] == '\n') {
			req.buf[i] = '*';
		}
	}

	String path = req_path(&req);

	log("Request: %.*s", path.len, path.chars);

	Handler h = get_handler(s, path);
	if (h == NULL) {
		h = not_found;
	}
	h(&req, &resp);

	dprintf(fd, "HTTP/1.1 %d OK\nServer: spence/1.0\nContent-Length: %d\nConnection: close\nContent-Type: text/html\n\n", resp.status, resp.bufi);
	dprintf(fd, "%s", resp.buf);
	log("Respond: %.*s %d: %d bytes", path.len, path.chars, resp.status, resp.bufi);

	//sleep(1); // Apparently to flush the socket?
	close(fd);
	return;
}

void serve_forever(Server s) {
	int listenfd;
	static struct sockaddr_in serv_addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		die("Failed to open socket");
	}

	serv_addr = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_addr = {
			.s_addr = htonl(INADDR_ANY),
		},
		.sin_port = htons(s->port),
	};

	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		die("Failed to bind socket to port");
	}

	if (listen(listenfd, 64) < 0) {
		die("Failed to listen to socket");
	}

	for (;;) {
		accept_and_handle(s, listenfd);
	}
}

/*
 * Internal handlers
 */

void not_found(Request req, Response resp) {
	resp_status(resp, 404);
	resp_write(resp, "Not found");
}

void accept_fork(Server s, int listenfd) {
	int socketfd, pid;
	if ((socketfd = accept(listenfd, NULL, NULL)) < 0) {
		log("Error accepting socket connection");
		return;
	}

	if ((pid = fork()) < 0) {
		log("Could not fork to handle request");
	}
	else {
		if (pid == 0) {
			// Child handles request
			(void) close(listenfd);
			handle_thread(s, socketfd);
			exit(EXIT_SUCCESS);
		}
		else {
			// Parent closes and moves to next
			(void) close(socketfd);
		}
	}
}

void accept_nofork(Server s, int listenfd) {
	int socketfd;
	if ((socketfd = accept(listenfd, NULL, NULL)) < 0) {
		log("Error accepting socket connection");
		return;
	}

	handle_thread(s, socketfd);
}
