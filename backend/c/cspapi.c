/* CSP catalog HTTP backend. Serves indexed packages, libraries, sources, and OAuth notices. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET csp_socket;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int csp_socket;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

static char root[1024];
static char oauth_store[2048];

static void join_root(char *out, size_t cap, const char *relative) {
  snprintf(out, cap, "%s/%s", root, relative);
}

static int send_all(csp_socket client, const char *data, int length) {
  int sent = 0;
  while (sent < length) {
    int n = send(client, data + sent, length - sent, 0);
    if (n <= 0)
      return 0;
    sent += n;
  }
  return 1;
}

static void send_response(csp_socket client, int status, const char *status_text,
                          const char *type, const char *body, int length) {
  char header[512];
  int header_len = snprintf(header, sizeof(header),
                            "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
                            "Access-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",
                            status, status_text, type, length);
  send_all(client, header, header_len);
  if (body && length)
    send_all(client, body, length);
}

static void send_file(csp_socket client, const char *relative, const char *type) {
  char path[2048];
  join_root(path, sizeof(path), relative);
  FILE *file = fopen(path, "rb");
  if (!file) {
    const char *missing = "{\"error\":\"not found\"}";
    send_response(client, 404, "Not Found", "application/json", missing,
                  (int)strlen(missing));
    return;
  }
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *body = (char *)malloc((size_t)size + 1);
  if (!body) {
    fclose(file);
    const char *fail = "{\"error\":\"out of memory\"}";
    send_response(client, 500, "Error", "application/json", fail, (int)strlen(fail));
    return;
  }
  fread(body, 1, (size_t)size, file);
  fclose(file);
  send_response(client, 200, "OK", type, body, (int)size);
  free(body);
}

static void save_oauth(const char *body, int length) {
  FILE *file = fopen(oauth_store, "ab");
  if (!file)
    return;
  fwrite(body, 1, (size_t)length, file);
  fputc('\n', file);
  fclose(file);
}

static void handle(csp_socket client) {
  char request[8192];
  int got = recv(client, request, sizeof(request) - 1, 0);
  if (got <= 0)
    return;
  request[got] = 0;
  char method[16] = {0};
  char path[1024] = {0};
  sscanf(request, "%15s %1023s", method, path);
  char *query = strchr(path, '?');
  if (query)
    *query = 0;

  if (strcmp(method, "POST") == 0 && strcmp(path, "/api/oauth/session") == 0) {
    char *payload = strstr(request, "\r\n\r\n");
    payload = payload ? payload + 4 : "";
    save_oauth(payload, (int)strlen(payload));
    const char *ok = "{\"ok\":true,\"backend\":\"c\"}";
    send_response(client, 200, "OK", "application/json", ok, (int)strlen(ok));
  } else if (strcmp(path, "/api/packages") == 0) {
    send_file(client, "website/data/packages.json", "application/json");
  } else if (strcmp(path, "/api/libraries") == 0) {
    send_file(client, "website/data/libraries.json", "application/json");
  } else if (strcmp(path, "/api/sources") == 0) {
    send_file(client, "website/data/sources.json", "application/json");
  } else if (strcmp(path, "/api/catalog") == 0) {
    send_file(client, "website/data/manifest.json", "application/json");
  } else if (strcmp(path, "/api/oauth/sessions") == 0) {
    send_file(client, "website/data/oauth-sessions.jsonl", "application/jsonl");
  } else {
    const char *help =
        "{\"service\":\"csp-c-backend\",\"routes\":[\"/api/catalog\",\"/api/"
        "packages\",\"/api/libraries\",\"/api/sources\",\"POST "
        "/api/oauth/session\"]}";
    send_response(client, 200, "OK", "application/json", help, (int)strlen(help));
  }
  closesocket(client);
}

int main(int argc, char **argv) {
  const char *port = argc > 1 ? argv[1] : "8081";
  if (argc > 2)
    snprintf(root, sizeof(root), "%s", argv[2]);
  else {
#ifdef _WIN32
    GetCurrentDirectoryA((DWORD)sizeof(root), root);
#else
    if (!getcwd(root, sizeof(root)))
      strcpy(root, ".");
#endif
  }
  join_root(oauth_store, sizeof(oauth_store), "website/data/oauth-sessions.jsonl");

#ifdef _WIN32
  WSADATA data;
  WSAStartup(MAKEWORD(2, 2), &data);
#endif
  csp_socket server = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons((unsigned short)atoi(port));
  if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
    perror("bind");
    return 1;
  }
  listen(server, 16);
  printf("C+ C backend http://127.0.0.1:%s\n", port);
  for (;;) {
    csp_socket client = accept(server, NULL, NULL);
    if (client == INVALID_SOCKET)
      continue;
    handle(client);
  }
}
