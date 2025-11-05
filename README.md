# CSOPESY MO1 — OS Emulator: Process Scheduler (Group 9)

An interactive, terminal-based OS emulator that simulates a multi-core CPU process scheduler. Supports FCFS and Round-Robin scheduling, process creation/inspection, live CPU utilization display, and utilization report generation.

- Course: CSOPESY | Section S13
- Assessment: MO1 — OS Emulator — Process Scheduler
- Developers: Alvarez, Ivan Antonio T.; Barlaan, Bahir Benjamin C.; Co, Joshua Benedict B.; Tan, Reyvin Matthew T.
- Last Updated: 2025-11-05

---

## Project Structure

```
CSOPESY-MCO1_Group-9-main (1)/
├─ CSOPESY-MCO1_Group-9-main/
│  └─ CSOPESY-MCO1-OS_Emulator-Process_Scheduler/
│     ├─ Group_9_MO1_OS_Emulator.cpp
│     ├─ CSOPESY-MCO1-OS_Emulator-Process_Scheduler.sln
│     ├─ CSOPESY-MCO1-OS_Emulator-Process_Scheduler.vcxproj*
│     ├─ config.txt (optional; runtime config)
│     ├─ config_rr.txt (sample RR config)
│     └─ ...
├─ MO1 - OS Emulator - Process Scheduler.pdf (spec/handout)
└─ README.md
```

> Primary source file: `CSOPESY-MCO1_Group-9-main/CSOPESY-MCO1-OS_Emulator-Process_Scheduler/Group_9_MO1_OS_Emulator.cpp`

---

## Features

- Multiple CPU cores (configurable)
- FCFS and Round-Robin schedulers (configurable quantum)
- Interactive command console with colored UI and live CPU stats
- Create/view/list processes; attach to a process screen
- Auto batch process generation on a timer
- CPU utilization and process summary report to file

---

## Prerequisites

- Windows 10/11, macOS, or Linux
- C++14-capable compiler
  - Windows (MSVC): Visual Studio 2019+ or Build Tools
  - Windows (MinGW): `g++` with pthreads
  - Linux/macOS: `g++` with pthreads

> On Windows terminals, ANSI color is enabled automatically when available.

---

## Build Instructions

Change directory to the source folder first:

```bash
cd "CSOPESY-MCO1_Group-9-main (1)/CSOPESY-MCO1_Group-9-main/CSOPESY-MCO1-OS_Emulator-Process_Scheduler"
```

- MSVC (Developer Command Prompt):

```bat
cl /EHsc /std:c++14 Group_9_MO1_OS_Emulator.cpp
```

- MinGW (Windows):

```bash
g++ -std=c++14 -pthread Group_9_MO1_OS_Emulator.cpp -o os_emulator.exe
```

- Linux/macOS:

```bash
g++ -std=c++14 -pthread Group_9_MO1_OS_Emulator.cpp -o os_emulator
```

---

## Run

From the same folder where you compiled:

- Windows (MSVC build):

```bat
Group_9_MO1_OS_Emulator.exe
```

- Windows (MinGW build):

```bat
os_emulator.exe
```

- Linux/macOS:

```bash
./os_emulator
```

---

## Configuration (config.txt)

Optional file placed next to the executable/source. If absent, safe defaults are used.

Example `config.txt`:

```
num-cpu 4
scheduler fcfs
quantum-cycles 5
min-ins 100
max-ins 1000
delays-per-exec 100
batch-process-freq 3
```

Parameters:
- num-cpu: number of CPU cores (>=1)
- scheduler: `fcfs` or `rr`
- quantum-cycles: time quantum for `rr` (>=1)
- min-ins / max-ins: instruction count range used when generating process programs
- delays-per-exec: ms delay per instruction (>=0)
- batch-process-freq: seconds between automatic process creation (>=1)

A sample Round-Robin config is included as `config_rr.txt`.

---

## Interactive Commands

Type these at the `CSOPESY>` prompt in the emulator UI:

- `help`: Show available commands
- `initialize`: Start the OS emulator and scheduler (run this first)
- `screen -s <name>`: Create a new process
- `screen -r <name>`: Attach to a process screen (type `exit` to return)
- `screen -ls`: List processes and states
- `scheduler-start`: Start auto process generation on a timer
- `scheduler-stop`: Stop auto generation (scheduler stays on)
- `report-util`: Generate CPU utilization report (`csopesy-log_YYYYMMDD_HHMMSS.txt`)
- `clear`: Redraw the main UI
- `exit`: Exit the emulator

Typical workflow:
1) `initialize`
2) `screen -s myProcess1`
3) `screen -ls`
4) `screen -r myProcess1` (then `exit`)
5) `report-util`
6) `exit`

---

## Notes and Tips

- Use a terminal window wide enough (recommend ≥120 cols) to view the UI cleanly.
- On Windows, prefer a terminal that supports ANSI colors (Windows Terminal, VS Code Terminal, etc.).
- If a process already finished, `screen -r <name>` won’t reattach; use `screen -ls` for summary instead.

---

## Troubleshooting

- Build errors on Windows (MinGW): ensure `-pthread` is included and your MinGW supports C++14.
- No colors on Windows: run in Windows Terminal or recent PowerShell; ANSI is enabled when possible.
- Report file not created: ensure write permissions in the working directory.
- Nothing happens after commands: confirm you ran `initialize` first.

---

## License

For academic use within CSOPESY. If you need a specific open-source license, add it here.
