#include "slughttp.h"

void root(Request req, Response resp) {
	resp_write(resp,
			"<form action='/hello' method='post'>"
			"Your name:<br>"
			"<input type='text' placeholder='First Last' name='name'><br>"
			"Your age:<br>"
			"<input type='number' name='age' placeholder='18'><br><br>"
			"<input type='submit' value='Submit'>"
			"</form>");
}

void hello(Request req, Response resp) {
	String name = req_post_arg(req, CString("name"));
	String age = req_post_arg(req, CString("age"));
	resp_write(resp, "Hello, %.*s. You are %.*s years old!",
			(int)name.len, name.chars,
			(int)age.len, age.chars);
}

int main() {
	Server s = new_server(8080, true);
	handle_path(s, "/", root);
	handle_path(s, "/hello", hello);
	serve_forever(s);
}
