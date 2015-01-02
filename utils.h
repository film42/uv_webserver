void ws_logger(const char * fmt, ...);

char * ws_timestamp(void);

/* URL N COMPARE
 * Compare the first n elements, or until a `?' is found */
int ws_urlncmp(const char * first, const char * second, size_t length);