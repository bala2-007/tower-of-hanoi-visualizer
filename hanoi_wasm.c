/* Tower of Hanoi solver in C, compiled to WebAssembly so it runs in the browser.
 * Same logic as server.c (stack ADT + recursion + validation), without the HTTP part.
 *
 * Build (Emscripten):
 *   emcc hanoi_wasm.c -O2 --no-entry -o hanoi.js -sMODULARIZE=1 -sEXPORT_NAME=createHanoi ^
 *        -sEXPORTED_FUNCTIONS=_solve,_move_disk,_move_from,_move_to -sEXPORTED_RUNTIME_METHODS=cwrap
 */
#include <emscripten.h>

#define MAX_DISKS 8
#define MAX_MOVES 255                 /* 2^8 - 1 */

/* ---------- Stack ADT: one stack per peg ---------- */
typedef struct { int data[MAX_DISKS]; int top; } Stack;

static void s_init(Stack *s)        { s->top = -1; }
static int  s_empty(const Stack *s) { return s->top < 0; }
static int  s_peek(const Stack *s)  { return s->data[s->top]; }
static void s_push(Stack *s, int d) { s->data[++s->top] = d; }
static int  s_pop(Stack *s)         { return s->data[s->top--]; }

/* ---------- Recursive solver ---------- */
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

/* Replays the moves on three stacks (pop, then push) and checks every rule. */
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

/* ---------- Functions called from JavaScript ---------- */
/* Solves for n disks (peg 0 -> peg 2). Returns the number of moves, or -1 on error. */
EMSCRIPTEN_KEEPALIVE int solve(int n)
{
    if (n < 3 || n > MAX_DISKS) return -1;
    nmoves = 0;
    hanoi(n, 0, 2, 1);
    return (nmoves == (1 << n) - 1 && simulate(n)) ? nmoves : -1;
}
EMSCRIPTEN_KEEPALIVE int move_disk(int i) { return moves[i].disk; }
EMSCRIPTEN_KEEPALIVE int move_from(int i) { return moves[i].from; }
EMSCRIPTEN_KEEPALIVE int move_to(int i)   { return moves[i].to; }
