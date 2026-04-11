# Process & Resource Manager

A simulation of a process and resource manager using PCBs (Process Control Blocks) and RCBs (Resource Control Blocks), implementing a multilevel priority-based scheduler.

## How to Compile

```bash
g++ -std=c++17 manager.cpp -o manager
```

Make sure `manager.cpp` and `manager.hpp` are in the same directory before compiling.

---

## How to Run

### Interactive (terminal input)

```bash
./manager
```

Type commands directly into the terminal, one per line. For example:
```
in
cr 1
cr 2
to
```

Press `Ctrl+D` to exit.

### From an input file

```bash
./manager input.txt
```

Where `input.txt` has one command per line. An empty line triggers a new `init`.

---

## Commands

| Command | Description |
|--------|-------------|
| `in` | Initialize the system |
| `cr <priority>` | Create a process with priority 0–3 |
| `de <pid>` | Destroy a process and its children |
| `rq <rid> <units>` | Request units of a resource |
| `rl <rid> <units>` | Release units of a resource |
| `to` | Timeout (round-robin the current process) |

Resources are indexed 0–3 with inventories of 1, 1, 2, and 3 units respectively.

---

## How to Compare Output Against a Sample

Save your program's output to a file, then diff it against the expected output:

```bash
./manager input.txt > my_output.txt
diff my_output.txt sample_output.txt
```

If `diff` produces no output, your output matches exactly. Any differences will be shown line by line.

---

## Example

**input.txt**
```
in
cr 1
cr 1
cr 1
to
to
rq 3 1
to
rq 3 2
to
to
rl 3 2
```

**Expected output**
```
0 1 1 1 2 3 3 1 1 2 3 -1
```