#ifndef OSLAB_HTTP_H
#define OSLAB_HTTP_H
#include "os.h"
#define HTTP_REQUEST_LIMIT 768
struct http_response {
  const char *data;
  size_t size;
};
// 0: incomplete, 1: complete response selected, -1: oversized/malformed.
int http_select(const char *, size_t, struct http_response *);
#endif
