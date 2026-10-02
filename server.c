/* Tower of Hanoi back end in C: recursion + stacks, served over HTTP.
 *
 * Build (Linux / macOS / WSL):  gcc -O2 -Wall server.c -o hanoi_server
 * Build (Windows, MinGW):       gcc -O2 -Wall server.c -o hanoi_server -lws2_32
 * Run (from the folder that contains tower-of-hanoi.html):  ./hanoi_server
 * Then open http://localhost:8080
 *
 * Endpoints:
 *   GET /                    -> serves tower-of-hanoi.html
 *   GET /api/solve?disks=N   -> {"disks":N,"optimal":2^N-1,"moves":[{"disk":k,"from":0,"to":2},...]}
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#define CLOSESOCK closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define CLOSESOCK close
#endif

#define PORT       8080
#define MAX_DISKS  8
#define MAX_MOVES  255            /* 2^8 - 1 */

/* ---------- Stack ADT: one stack per peg ---------- */
typedef struct { int data[MAX_DISKS]; int top; } Stack;

static void s_init(Stack *s)        { s->top = -1; }
static int  s_empty(const Stack *s) { return s->top < 0; }
static int  s_peek(const Stack *s)  { return s->data[s->top]; }
static void s_push(Stack *s, int d) { s->data[++s->top] = d; }
static int  s_pop(Stack *s)         { return s->data[s->top--]; }

/* ---------- Recursive solver: records every move in order ---------- */
typedef struct { int disk, from, to; } Move;
static Move moves[MAX_MOVES];
static int  nmoves;

static void hanoi(int k, int from, int to, int aux)
{
    if (k == 0) return;                      /* base case */
    hanoi(k - 1, from, aux, to);             /* 1) park k-1 smaller disks on the spare peg */
    moves[nmoves].disk = k;                  /* 2) move the largest disk */
    moves[nmoves].from = from;
    moves[nmoves].to   = to;
    nmoves++;
    hanoi(k - 1, aux, to, from);             /* 3) rebuild k-1 disks on top of it */
}

/* Replays the move list on three stacks (pop, then push) and checks the rules.
 * Returns 1 if every move was legal and all disks end on peg 2. */
static int simulate(int n)
{
    Stack t[3];
    int i;
    for (i = 0; i < 3; i++) s_init(&t[i]);
    for (i = n; i >= 1; i--) s_push(&t[0], i);

    for (i = 0; i < nmoves; i++) {
        Move m = moves[i];
        if (s_empty(&t[m.from]) || s_peek(&t[m.from]) != m.disk) return 0;
        if (!s_empty(&t[m.to]) && s_peek(&t[m.to]) < m.disk)     return 0;
        s_push(&t[m.to], s_pop(&t[m.from]));
    }
    return t[2].top == n - 1;
}

/* ---------- Minimal HTTP layer ---------- */
static void send_all(SOCKET c, const char *buf, size_t len)
{
    while (len > 0) {
        int w = (int)send(c, buf, (int)len, 0);
        if (w <= 0) return;
        buf += w;
        len -= (size_t)w;
    }
}

static void respond(SOCKET c, int code, const char *type, const char *body, size_t len)
{
    char head[256];
    const char *reason = code == 200 ? "OK" : code == 400 ? "Bad Request"
                       : code == 404 ? "Not Found" : "Internal Server Error";
    int h = snprintf(head, sizeof head,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",
        code, reason, type, (unsigned long)len);
    send_all(c, head, (size_t)h);
    send_all(c, body, len);
}

static void respond_text(SOCKET c, int code, const char *type, const char *msg)
{
    respond(c, code, type, msg, strlen(msg));
}

static void serve_index(SOCKET c)
{
    FILE *f = fopen("tower-of-hanoi.html", "rb");
    char *buf;
    long len;
    size_t got;
    if (!f) {
        respond_text(c, 404, "text/plain", "tower-of-hanoi.html not found. Run the server from its folder.");
        return;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    buf = (char *)malloc((size_t)len);
    if (!buf) { fclose(f); respond_text(c, 500, "text/plain", "Out of memory"); return; }
    got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    respond(c, 200, "text/html; charset=utf-8", buf, got);
    free(buf);
}

static void serve_solve(SOCKET c, const char *path)
{
    char json[10240];
    const char *p = strstr(path, "disks=");
    int n = p ? atoi(p + 6) : 0;
    int off, i;

    if (n < 3 || n > MAX_DISKS) {
        respond_text(c, 400, "application/json", "{\"error\":\"disks must be between 3 and 8\"}");
        return;
    }

    nmoves = 0;
    hanoi(n, 0, 2, 1);                       /* peg 0 = A (source), 1 = B (aux), 2 = C (target) */
    if (nmoves != (1 << n) - 1 || !simulate(n)) {
        respond_text(c, 500, "application/json", "{\"error\":\"solver failed validation\"}");
        return;
    }

    off = snprintf(json, sizeof json, "{\"disks\":%d,\"optimal\":%d,\"moves\":[", n, (1 << n) - 1);
    for (i = 0; i < nmoves; i++)
        off += snprintf(json + off, sizeof json - (size_t)off,
                        "%s{\"disk\":%d,\"from\":%d,\"to\":%d}",
                        i ? "," : "", moves[i].disk, moves[i].from, moves[i].to);
    off += snprintf(json + off, sizeof json - (size_t)off, "]}");
    respond(c, 200, "application/json", json, (size_t)off);
}

int main(void)
{
    SOCKET srv;
    struct sockaddr_in addr;
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET) { perror("socket"); return 1; }
#ifndef _WIN32
    { int yes = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes); }
#endif
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   /* local machine only */
    addr.sin_port        = htons(PORT);
    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(srv, 8) < 0) {
        perror("bind/listen (is port 8080 already in use?)");
        return 1;
    }
    printf("Hanoi back end running: http://localhost:%d\n", PORT);

    for (;;) {
        char req[2048], method[8], path[256];
        int r;
        SOCKET c = accept(srv, NULL, NULL);
        if (c == INVALID_SOCKET) continue;

        r = (int)recv(c, req, sizeof req - 1, 0);
        if (r > 0) {
            req[r] = '\0';
            if (sscanf(req, "%7s %255s", method, path) == 2 && strcmp(method, "GET") == 0) {
                if (strncmp(path, "/api/solve", 10) == 0)                         serve_solve(c, path);
                else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) serve_index(c);
                else respond_text(c, 404, "text/plain", "Not found");
            } else {
                respond_text(c, 400, "text/plain", "Only GET is supported");
            }
        }
        CLOSESOCK(c);
    }
    return 0;
}
