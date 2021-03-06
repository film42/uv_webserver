#include "webserver.h"

uv_buf_t home_route_cb(void) {
  return ws_http_response(200,
      "<center>"
          "<h3>Home page!</h3>"
          "<marquee>Hello, welcome to my libuv webapp!</marquee>"
      "</center>"
  );
}

uv_buf_t about_route_cb(void) {
  return ws_http_response(200, "Hello, World!");
}

uv_buf_t hannah_route_cb(void) {
  return ws_http_response(200, "Hooolllaaa! :) :) :)");
}

int main(int argc, char const *argv[]) {
  ws_init();

  ws_get("/", home_route_cb);

  ws_get("/about", about_route_cb);

  ws_get("/hannah", hannah_route_cb);
  ws_get("/hannah/me", hannah_route_cb);

  return ws_start();
}