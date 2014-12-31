#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "webserver.h"

#define ws_logger printf

uv_loop_t * loop;

typedef struct {
  char *        path;
  on_request_cb on_req_cb;
} route_t;

route_t * routes;
ssize_t route_count;

typedef struct {
  char * path;
} request_t;

void ws_free_on_write_cb(uv_buf_t *buf) {
  free(buf->base);
}

void ws_get(char * route, on_request_cb request_cb) {
  route_t new_route;
  new_route.path = route;
  new_route.on_req_cb = request_cb;

  routes[route_count] = new_route;
  route_count++;
}

uv_buf_t ws_http_response(int status, char * response) {
  char * status_message;

  switch(status){
    case 200: status_message = "HTTP/1.1 200 OK\r\n"; break;
    default: status_message = "HTTP/1.1 404 Not Found\r\n"; break;
  }

  char * headers = "Content-Type: text/html; charset=UTF-8\r\n"
                   "Content-Length: ";

  ssize_t response_length = strlen(response);
  ssize_t content_length_size = (ssize_t)floor(log10(response_length)) + 1;
  /* status message + Actual response + headers + content length size + 6 for new lines "\r\n" */
  ssize_t message_size = strlen(status_message) + response_length + strlen(headers) + content_length_size + 6;

  char * buffer = (char *)malloc(sizeof(char) * message_size);

  sprintf(buffer, "%s%s%zu\r\n\r\n%s\r\n", status_message, headers, response_length, response);

  return uv_buf_init(buffer, message_size);
}

static void on_write(uv_write_t *req, int status) {
  if(status) {
    fprintf(stderr, "Write error: %s\n", uv_err_name(status));
  }

  free(req);
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if(nread < 0) {
    if(nread != UV_EOF) {
      fprintf(stderr, "Read error: %s!\n", uv_err_name((int)nread));
    }

    uv_close((uv_handle_t *) client, NULL);
    return;
  }

  /* Find the requested route */
  char * cursor_start;
  char * cursor_end;
  cursor_start = strchr(buf->base, ' ');
  cursor_end = strchr(cursor_start + 1, ' ');

  /* Get the route length so we can copy it */
  ssize_t route_length = (cursor_end - buf->base) - (cursor_start + 1 - buf->base);
  char * route_buffer = (char *) malloc(sizeof(char) * (route_length + 1));
  route_buffer[route_length + 1] = '\0';
  strncpy(route_buffer, (cursor_start + 1), route_length);

//  printf("%d %d %d\n", (cursor_start + 1 - buf->base), (cursor_end - buf->base), route_length);

  ws_logger("[PATH: '%s']\n", route_buffer);

  /* Match route */
  int i = 0;
  for(; i < route_count; ++i) {
    if(strcmp(routes[i].path, route_buffer) == 0) {
      uv_buf_t resp_buf = routes[i].on_req_cb();

      uv_write_t * req = (uv_write_t *) malloc(sizeof(uv_write_t));

      uv_write(req, client, &resp_buf, 1, on_write);
      free(buf->base);
      free(route_buffer);
      return;
    }
  }

  /* No route match */
  uv_write_t * req = (uv_write_t *) malloc(sizeof(uv_write_t));

  uv_buf_t resp_buf = ws_http_response(404, "404 - Route not found");

  uv_write(req, client, &resp_buf, 1, on_write);
  free(buf->base);
  free(route_buffer);
}

static void on_alloc(uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf) {
  buf->base = (char *) malloc(suggested_size);
  buf->len = suggested_size;
}

static void on_new_connection(uv_stream_t * server, int status) {
  if(status != 0) {
    fprintf(stderr, "Error in on_new_connection: %s", uv_err_name(status));
  }

  uv_tcp_t * client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);

  if(uv_accept(server, (uv_stream_t *) client) == 0) {
    uv_read_start((uv_stream_t *) client, on_alloc, on_read);
  } else {
    uv_close((uv_handle_t *)client, NULL);
  }
}

int ws_init() {
  // Allow default 1,000 routes
  routes = (route_t *)malloc(sizeof(route_t) * 1000);
  route_count = 0;

  return 0;
}

int ws_start() {
  loop = uv_default_loop();

  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  struct sockaddr_in bind_addr;
  assert(uv_ip4_addr("0.0.0.0", 3000, &bind_addr) == 0);

  uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);

  int status = uv_listen((uv_stream_t *)&server, 128, on_new_connection);

  if(status) {
    fprintf(stderr, "Listen error!: %s\n", uv_err_name(status));
  } else {
    printf("Started socket on port %d\n", 3000);
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}