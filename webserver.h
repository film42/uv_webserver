#include <uv.h>

typedef uv_buf_t (* on_request_cb)(void);

int ws_init();

int ws_start();

uv_buf_t ws_http_response(int status, char * response);

void ws_get(char * route, on_request_cb request_cb);
