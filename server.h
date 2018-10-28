#include <stdio.h>

typedef struct ResponseObj *Response;
typedef struct RequestObj *Request;
typedef struct ServerObj* Server;

typedef void(*Handler)(Request, Response);

typedef struct String {
	char *chars;
	size_t len;
} String;

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

Server new_server(int port);
int handle_path(Server s, const char *path, Handler h);
Handler get_handler(Server s, String path);
void serve_forever(Server s);
