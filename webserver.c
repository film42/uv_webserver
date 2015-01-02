#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "webserver.h"
#include "utils.h"
#include "deps/http_parser.h"

static uv_loop_t * loop;
static http_parser_settings parser_settings;

typedef struct {
  uv_tcp_t    handle;
  http_parser parser;
  uv_write_t  write_req;
} client_t;

typedef struct {
  char *        path;
  size_t        length;
  on_request_cb on_req_cb;
} route_t;

route_t * routes;
ssize_t route_count;

void ws_free_on_write_cb(uv_buf_t *buf) {
  free(buf->base);
}

void ws_get(char * route, on_request_cb request_cb) {
  route_t new_route;
  new_route.path = route;
  new_route.length = strlen(route);
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

static void on_close(uv_handle_t * handle) {
  client_t * client = (client_t *) handle->data;

  free(client);
}

static void on_write(uv_write_t *req, int status) {
  if(status) {
    fprintf(stderr, "Write error: %s\n", uv_err_name(status));
  }

  free(req);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {

  client_t * client = (client_t *) stream->data;

  if(nread >= 0) {
    size_t parsed = http_parser_execute(&client->parser, &parser_settings, buf->base, nread);

    if(parsed < nread) {
      fprintf(stderr, "Parse Error!");
      uv_close((uv_handle_t *) &client->handle, on_close);
    }
  } else {
    if(nread != UV_EOF) {
      fprintf(stderr, "Read error: %s!\n", uv_err_name((int)nread));
    }
  }
}

static void on_alloc(uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf) {
  buf->base = (char *) malloc(suggested_size);
  buf->len = suggested_size;
}

static void on_new_connection(uv_stream_t * server, int status) {
  if(status != 0) {
    fprintf(stderr, "Error in on_new_connection: %s", uv_err_name(status));
  }

  client_t* client = malloc(sizeof(client_t));

  uv_tcp_init(loop, &client->handle);

  http_parser_init(&client->parser, HTTP_REQUEST);

  client->parser.data = client;
  client->handle.data = client;

  if(uv_accept(server, (uv_stream_t *) &client->handle) == 0) {
    uv_read_start((uv_stream_t *) &client->handle, on_alloc, on_read);
  } else {
    uv_close((uv_handle_t *) &client->handle, NULL);
  }
}

static int on_url(http_parser * parser, const char * at, size_t length) {

  /* Recover the client_t pointer */
  client_t* client = (client_t *) parser->data;

  char buffer[length + 1];
  strncpy(buffer, at, length);
  buffer[length + 1] = '\0';

  ws_logger("[time: %s, path: '%s']\n", ws_timestamp(), &buffer);

  /* Match route */
  int i = 0;
  for(; i < route_count; ++i) {
    /* Match characters against `at' - the starting position of the url */
    if(ws_urlncmp(routes[i].path, at, length) == 0) {
      uv_buf_t resp_buf = routes[i].on_req_cb();

      uv_write_t * req = (uv_write_t *) malloc(sizeof(uv_write_t));

      uv_write(req, (uv_stream_t *) &client->handle, &resp_buf, 1, on_write);
      return 0;
    }
  }

  /* No route match */
  uv_write_t * req = (uv_write_t *) malloc(sizeof(uv_write_t));

  uv_buf_t resp_buf = ws_http_response(404, "Route not found");

  uv_write(req, (uv_stream_t *) &client->handle, &resp_buf, 1, on_write);

  return 0;
}

int ws_init() {
  // Allow default 1,000 routes
  routes = (route_t *)malloc(sizeof(route_t) * 1000);
  route_count = 0;

  return 0;
}

int ws_start() {
  /* Using http parser - set callbacks */
  parser_settings.on_url = on_url;

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