#include <stdio.h>
#include <stdbool.h>

typedef struct ResponseObj *Response;
typedef struct RequestObj *Request;
typedef struct ServerObj* Server;

typedef void(*Handler)(Request, Response);

// A String wrapper is helpful for passing around slices of buffers.
// A macro CString is provided to turn constant char arrays into Strings.
typedef struct String {
	char *chars;
	size_t len;
} String;
#define CString(X) (String){.chars=X,.len=sizeof(X)-1}

#define resp_write(R, ...) do{\
	int _bytes_written = snprintf(resp_buf(R), resp_nleft(R), __VA_ARGS__);\
	resp_wrote_n(R, _bytes_written);\
}while(0)
char *resp_buf(Response r);
int resp_nleft(Response r);
void resp_wrote_n(Response r, size_t wrote);
void resp_status(Response r, int n);

String req_path(Request r);
String req_post_arg(Request r, String arg_name);

typedef struct ServerOpts {
	bool multiprocess;
	bool handle_sigint;
	bool enable_logging;
} ServerOpts;

Server new_server(int port, struct ServerOpts opts);
int handle_path(Server s, const char *path, Handler h);
Handler get_handler(Server s, String path);
void serve_forever(Server s);
