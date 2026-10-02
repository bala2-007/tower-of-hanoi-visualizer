# Tower of Hanoi Visualizer

**Live demo:** https://tower-of-hanoi-visualizer-seven.vercel.app/

A Data Structures and Algorithms mini-project: an animated Tower of Hanoi solver.
The front end is HTML5, CSS3 and vanilla JavaScript. The solver is written in **C**: recursion generates the moves, and stacks validate them.

## Features

* Choose 3 to 8 disks, with Slow, Medium and Fast animation speeds
* Start simulation and Reset buttons (controls are disabled while a run is in progress)
* Move counter compared with the optimal 2^n − 1, plus a progress bar
* Live stack contents shown under each peg
* Completion pop-up showing the total moves
* Collapsible time and space complexity explanation

## How it works

1. The C solver runs the recursive function `hanoi(k, from, to, aux)`, which records every move in order.
2. It replays those moves on three array-based stacks (`push` and `pop`) to confirm every move is legal and all disks end on peg C.
3. The browser animates each move: pop the disk from the source stack, lift it, slide it across, drop it, and push it onto the target stack.

The same C logic can be run in two ways:

|Version|Where the C code runs|Files|
|-|-|-|
|**Online (Vercel)**|In the browser, compiled to WebAssembly|`index.html`, `hanoi\\\\\\\_wasm.c`, `hanoi.js`, `hanoi.wasm`|
|**Local server**|As a small C HTTP server on your machine|`server.c`, `tower-of-hanoi.html`|

## Run the online version locally

The WebAssembly files must be served over HTTP (opening `index.html` directly won't work):

```
python -m http.server 8000
```

Then open **http://localhost:8000**.

To rebuild the WebAssembly files after changing `hanoi\\\\\\\_wasm.c` (needs [Emscripten](https://emscripten.org)):

```
emcc hanoi\\\\\\\_wasm.c -O2 --no-entry -o hanoi.js -sMODULARIZE=1 -sEXPORT\\\\\\\_NAME=createHanoi -sEXPORTED\\\\\\\_FUNCTIONS=\\\\\\\_solve,\\\\\\\_move\\\\\\\_disk,\\\\\\\_move\\\\\\\_from,\\\\\\\_move\\\\\\\_to -sEXPORTED\\\\\\\_RUNTIME\\\\\\\_METHODS=cwrap
```

## Run the local C server version

You need a C compiler (for example MinGW-w64 on Windows). In the project folder:

**Windows**

```
gcc -O2 -Wall server.c -o hanoi\\\\\\\_server -lws2\\\\\\\_32
hanoi\\\\\\\_server
```

**Linux / macOS**

```
gcc -O2 -Wall server.c -o hanoi\\\\\\\_server
./hanoi\\\\\\\_server
```

Open **http://localhost:8080** and keep the terminal open while you use the page.

## Complexity

* **Time: O(2^n).** The recurrence T(n) = 2·T(n−1) + 1 gives exactly 2^n − 1 moves, and no solution can use fewer.
* **Space: O(n).** The recursion is n calls deep, and the three stacks together never hold more than n disks.

|Disks|Moves (2^n − 1)|
|-|-|
|3|7|
|5|31|
|8|255|

## Built with

C (stack ADT, recursion, sockets), WebAssembly (Emscripten), HTML5, CSS3, JavaScript, Vercel

