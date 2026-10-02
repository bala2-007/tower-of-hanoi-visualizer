# Tower of Hanoi Visualizer

A Data Structures and Algorithms mini-project: an animated Tower of Hanoi solver.
The front end is HTML5, CSS3 and vanilla JavaScript. The back end is written in **C**: recursion generates the moves, and stacks validate them.

## Features

- Choose 3 to 8 disks, with Slow, Medium and Fast animation speeds
- Start simulation and Reset buttons (controls are disabled while a run is in progress)
- Move counter compared with the optimal 2^n − 1, plus a progress bar
- Live stack contents shown under each peg
- Completion pop-up showing the total moves
- Collapsible time and space complexity explanation

## How it works

1. The browser requests `GET /api/solve?disks=N` from the C server.
2. The server runs the recursive solver `hanoi(k, from, to, aux)`, which records every move in order.
3. The server replays those moves on three array-based stacks (`push` and `pop`) to confirm every move is legal and all disks end on peg C. It then returns the moves as JSON.
4. The browser animates each move: pop the disk from the source stack, lift it, slide it across, drop it, and push it onto the target stack.

Example response for 3 disks (shortened):

```json
{"disks":3,"optimal":7,"moves":[{"disk":1,"from":0,"to":2},{"disk":2,"from":0,"to":1}, ...]}
```

## Project structure

| File | Purpose |
|------|---------|
| `server.c` | C back end: stack ADT, recursive solver, move validation, small HTTP server |
| `tower-of-hanoi.html` | Front end: interface, animation and controls (HTML, CSS, JavaScript in one file) |

## Run it

You need a C compiler. On Windows, one option is MinGW-w64 via WinLibs:

```
winget install BrechtSanders.WinLibs.POSIX.UCRT
```

Open a terminal in the project folder, then:

**Windows**
```
gcc -O2 -Wall server.c -o hanoi_server -lws2_32
hanoi_server
```

**Linux / macOS**
```
gcc -O2 -Wall server.c -o hanoi_server
./hanoi_server
```

Open **http://localhost:8080** in your browser. Keep the terminal window open while you use the page.

> The C server has to be running for the visualizer to work, so this project can't be hosted on GitHub Pages (which only serves static files).

## Complexity

- **Time: O(2^n).** The recurrence T(n) = 2·T(n−1) + 1 gives exactly 2^n − 1 moves, and no solution can use fewer.
- **Space: O(n).** The recursion is n calls deep, and the three stacks together never hold more than n disks.

| Disks | Moves (2^n − 1) |
|-------|-----------------|
| 3 | 7 |
| 5 | 31 |
| 8 | 255 |

## Built with

C (sockets), HTML5, CSS3, JavaScript
