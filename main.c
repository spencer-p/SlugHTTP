#include "slughttpd.h"

void root(Request req, Response resp) {
	resp_write(resp, "Hello, World!");
}

int main() {
	Server s = new_server(8080, true);
	handle_path(s, "/", root);
	serve_forever(s);
}
