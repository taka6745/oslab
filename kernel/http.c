#include "http.h"
#define PAGE                                                                   \
  "<!doctype html><html lang=en><meta charset=utf-8><meta name=viewport "      \
  "content"                                                                    \
  "='width=device-width,initial-scale=1'><title>oslab / Independent "          \
  "computing</"                                                                \
  "title><style>body{margin:0 "                                                \
  "auto;padding:28px;max-width:900px;background:#f6f"                          \
  "3ec;color:#253128;font:16px/1.6 "                                           \
  "system-ui}header,footer{display:flex;justify"                               \
  "-content:space-between}small,footer{font:12px/1.6 "                         \
  "monospace;color:#526657}a{"                                                 \
  "color:inherit;text-underline-offset:5px}a:focus-visible{outline:2px "       \
  "solid}h1"                                                                   \
  "{font:clamp(48px,8vw,88px)/1.05 "                                           \
  "Georgia,serif;letter-spacing:-3px;margin:50p"                               \
  "x 0 24px}p{max-width:440px}section{border-block:1px solid "                 \
  "#bcc5b8;margin:48p"                                                         \
  "x 0 24px;padding:20px 0;display:flex;gap:24px;flex-wrap:wrap}section "      \
  "p{flex:"                                                                    \
  "1;min-width:180px;margin:0}b{display:block;font-size:18px}footer{gap:16px;" \
  "fl"                                                                         \
  "ex-wrap:wrap}</style><header><strong>oslab.</strong><a href=#inside>Under " \
  "th"                                                                         \
  "e hood</a></header><main><h1>Small by design.<br>Built from the "           \
  "metal.</h1><"                                                               \
  "p>An independent operating system. This page travels through a kernel, "    \
  "netwo"                                                                      \
  "rk stack and driver we wrote ourselves.<section id=inside><p><small>01 / "  \
  "FOU"                                                                        \
  "NDATION</small><b>Our own code.</b>Boot to browser, no guest "              \
  "libraries.<p><s"                                                            \
  "mall>02 / DELIVERY</small><b>Just the essentials.</b>One page. No scripts " \
  "or"                                                                         \
  " downloads.</section></main><footer><span>BARE METAL / OPEN "               \
  "SOURCE</span><a "                                                           \
  "href=https://github.com/taka6745/oslab>Read the source</a></footer></html>"
// Compile-time lengths and byte alignment keep the wire response contiguous.
#define RESPONSE(name, status, fields, body)                                   \
  static const struct {                                                        \
    char prefix[sizeof(status "\r\n" fields "Content-Length:") - 1]            \
        __attribute__((nonstring));                                            \
    char length[4];                                                            \
    char suffix[sizeof("\r\nConnection:close\r\n\r\n" body) - 1]               \
        __attribute__((nonstring));                                            \
  } name = {status "\r\n" fields "Content-Length:",                            \
            {'0' + (sizeof(body) - 1) / 1000 % 10,                             \
             '0' + (sizeof(body) - 1) / 100 % 10,                              \
             '0' + (sizeof(body) - 1) / 10 % 10,                               \
             '0' + (sizeof(body) - 1) % 10},                                   \
            "\r\nConnection:close\r\n\r\n" body};                              \
  _Static_assert(sizeof(body) - 1 <= 9999, "response length field capacity");  \
  _Static_assert(sizeof(name) ==                                               \
                     sizeof(status "\r\n" fields "Content-Length:") +          \
                         sizeof("\r\nConnection:close\r\n\r\n" body) + 2,      \
                 "contiguous wire layout")
// Empty optional reason phrase keeps the page within one 1460-byte MSS.
RESPONSE(page, "HTTP/1.0 200 ", "Content-Type:text/html;charset=utf-8\r\n",
         PAGE);
RESPONSE(missing, "HTTP/1.0 404 Not Found", "", "Not found\n");
RESPONSE(method, "HTTP/1.0 405 Method Not Allowed", "Allow: GET\r\n",
         "GET required\n");
RESPONSE(invalid, "HTTP/1.0 400 Bad Request", "", "Bad request\n");
#if !OSLAB_MACHINE_HTTP
static bool matches(const char *p, size_t n, const char *text) {
  size_t len = strlen(text);
  return n == len && !memcmp(p, text, n);
}
#endif
#if !OSLAB_MACHINE_HEADERS
static bool named(const char *p, size_t n, const char *name) {
  if (n != strlen(name))
    return false;
  for (size_t i = 0; i < n; i++) {
    char c = p[i];
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';
    if (c != name[i])
      return false;
  }
  return true;
}
static bool token(char c) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9'))
    return true;
  const char *punct = "!#$%&'*+-.^_`|~";
  while (*punct)
    if (*punct++ == c)
      return true;
  return false;
}
#if OSLAB_MACHINE_HTTP
bool machine_headers_valid(const char *p, size_t line, size_t end, bool http11) {
#else
static bool headers_valid(const char *p, size_t line, size_t end, bool http11) {
#endif
  bool host = false, length = false;
  for (size_t i = 0; i < line; i++)
    if ((uint8_t)p[i] < 32 || (uint8_t)p[i] > 126)
      return false;
  for (size_t cursor = line + 2; cursor < end;) {
    size_t stop = cursor;
    while (stop < end && (p[stop] != '\r' || p[stop + 1] != '\n'))
      stop++;
    size_t colon = cursor;
    while (colon < stop && p[colon] != ':') {
      char c = p[colon];
      if (!token(c))
        return false;
      colon++;
    }
    if (colon == cursor || colon == stop)
      return false;
    size_t first = colon + 1, last = stop;
    while (first < last && (p[first] == ' ' || p[first] == '\t'))
      first++;
    while (last > first && (p[last - 1] == ' ' || p[last - 1] == '\t'))
      last--;
    for (size_t i = first; i < last; i++)
      if (((uint8_t)p[i] < 32 && p[i] != '\t') || (uint8_t)p[i] > 126)
        return false;
    if (named(p + cursor, colon - cursor, "host")) {
      if (host || first == last)
        return false;
      for (size_t i = first; i < last; i++)
        if (p[i] == ' ' || p[i] == '\t')
          return false;
      host = true;
    } else if (named(p + cursor, colon - cursor, "transfer-encoding"))
      return false;
    else if (named(p + cursor, colon - cursor, "content-length")) {
      if (length || first == last)
        return false;
      for (size_t i = first; i < last; i++)
        if (p[i] != '0')
          return false; // This server accepts no request body.
      length = true;
    }
    cursor = stop + 2;
  }
  return !http11 || host;
}
#endif
#if OSLAB_MACHINE_HTTP
const struct http_response machine_http_responses[4] = {
    {(const char *)&invalid, sizeof(invalid)},
    {(const char *)&method, sizeof(method)},
    {(const char *)&missing, sizeof(missing)},
    {(const char *)&page, sizeof(page)}};
#else
int http_select(const char *p, size_t n, struct http_response *out) {
  if (n > HTTP_REQUEST_LIMIT)
    return -1;
  size_t end = 0;
  while (end + 3 < n && memcmp(p + end, "\r\n\r\n", 4))
    end++;
  if (end + 3 >= n)
    return 0;
  size_t line = 0;
  while (line + 1 < n && memcmp(p + line, "\r\n", 2))
    line++;
  size_t a = 0, b;
  while (a < line && p[a] != ' ')
    a++;
  b = a + 1;
  while (b < line && p[b] != ' ')
    b++;
  const char *response = (const char *)&invalid;
  size_t length = sizeof(invalid);
  if (n == end + 4 && a && a < b && b < line && p[a + 1] == '/' &&
      (matches(p + b + 1, line - b - 1, "HTTP/1.0") ||
       matches(p + b + 1, line - b - 1, "HTTP/1.1")) &&
      headers_valid(p, line, end,
                    matches(p + b + 1, line - b - 1, "HTTP/1.1"))) {
    if (!matches(p, a, "GET")) {
      response = (const char *)&method;
      length = sizeof(method);
    } else if (!matches(p + a + 1, b - a - 1, "/")) {
      response = (const char *)&missing;
      length = sizeof(missing);
    } else {
      response = (const char *)&page;
      length = sizeof(page);
    }
  }
  *out = (struct http_response){response, length};
  return 1;
}
#endif
