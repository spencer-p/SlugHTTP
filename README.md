# SlugHTTP

SlugHTTP is a minimal implementation of an HTTP library in C. It makes setting
up a simple web server in your ~~least~~ favorite language easy:

```c
#include "slughttp.h"

void root(Request req, Response resp) {
	resp_write(resp, "Hello, World!");
}

int main() {
	Server s = new_server(8080, true);
	handle_path(s, "/", root);
	serve_forever(s);
}
```

That's pretty cool! Unfortunately, that's about it. Performance is not optimal
(_sluggish_ one might say), and has tight limitations (8 kilobyte maximum
requests and responses, among other things).

For spinning up a dead simple web server or just seeing how a web server works
with raw sockets.
