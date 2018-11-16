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

That's pretty cool! For now, there are some tight limitations that I'm steadily
working on removing. But it's great for spinning up a dead simple web server or
just taking a look at how a web server works with raw sockets.

## Contributing

Open to pull requests!

## Name

It's mostly reference to UC Santa Cruz.
