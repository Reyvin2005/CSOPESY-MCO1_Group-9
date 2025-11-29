# MO2 OS Emulator - Memory Management System
**Group 9 - CSOPESY Machine Problem 2**  
**Date: November 29, 2025**

---

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Requirements](#requirements)
4. [Installation](#installation)
5. [Configuration](#configuration)
6. [Usage](#usage)
7. [Commands Reference](#commands-reference)
8. [Test Cases](#test-cases)
9. [Architecture](#architecture)
10. [Known Limitations](#known-limitations)

---

## Overview

MO2 is an enhanced operating system emulator that extends MO1 with comprehensive memory management capabilities. It simulates a multi-CPU scheduling system with virtual memory, paging, and LRU page replacement algorithms.

### What's New in MO2?
- **Paging System**: Configurable frame sizes with virtual memory support
- **LRU Algorithm**: Intelligent page replacement when memory is full
- **Backing Store**: Simulated virtual memory for paged-out pages
- **Memory Tracking**: Per-process memory allocation and usage statistics
- **New Commands**: vmstat, process-smi, scheduler-test
- **Automated Testing**: Built-in test automation for validation

---

## Features

### Memory Management
-  Dynamic memory allocation per process
-  Paging system with configurable frame sizes
-  LRU (Least Recently Used) page replacement
-  Backing store simulation
-  Per-process page tables
-  Page-in/page-out tracking

### Scheduling
-  Multi-CPU support (configurable cores)
-  FCFS (First-Come-First-Served) scheduling
-  Round-Robin scheduling with quantum
-  Automatic process generation
-  Process state management

### Diagnostics
-  Virtual memory statistics (vmstat)
-  Process memory information (process-smi)
-  CPU utilization tracking
-  Paging activity monitoring
-  Utilization reports

---

## Requirements

### Software
- **Compiler**: g++ with C++14 support or MSVC
- **Operating System**: Windows, Linux, or macOS
- **Libraries**: Standard C++ library, pthread

### Hardware
- Minimum 2 CPU cores recommended
- 1 GB RAM minimum

---

## Installation

### Windows (MinGW)
\\\powershell
g++ -std=c++14 -pthread -O2 Group_9_MO2_OS_Emulator.cpp -o mo2_emulator.exe
\\\

### Windows (MSVC)
\\\cmd
cl /EHsc /std:c++14 Group_9_MO2_OS_Emulator.cpp
\\\

### Linux/macOS
\\\ash
g++ -std=c++14 -pthread -O2 Group_9_MO2_OS_Emulator.cpp -o mo2_emulator
chmod +x mo2_emulator
\\\

---

## Configuration

Create a \config.txt\ file in the same directory as the executable:

\\\
num-cpu 4
scheduler rr
quantum-cycles 5
max-overall-mem 32768
mem-per-frame 32
min-mem-per-proc 8
max-mem-per-proc 8
min-ins 100
max-ins 1000
delays-per-exec 0
batch-process-freq 1
\\\

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| \
um-cpu\ | int | 4 | Number of CPU cores |
| \scheduler\ | string | "fcfs" | Scheduling algorithm ("fcfs" or "rr") |
| \quantum-cycles\ | int | 5 | Time quantum for Round-Robin |
| \max-overall-mem\ | size_t | 32768 | Total system memory (KB) |
| \mem-per-frame\ | size_t | 32 | Memory per frame/page (KB) |
| \min-mem-per-proc\ | size_t | 8 | Minimum memory per process (KB) |
| \max-mem-per-proc\ | size_t | 8 | Maximum memory per process (KB) |
| \min-ins\ | int | 100 | Minimum instructions per process |
| \max-ins\ | int | 1000 | Maximum instructions per process |
| \delays-per-exec\ | int | 100 | Delay in ms per instruction |
| \atch-process-freq\ | int | 3 | Frequency (seconds) for auto process creation |

---

## Usage

### Quick Start

1. **Compile the program** (see Installation)
2. **Create config.txt** with desired parameters
3. **Run the emulator**:
   \\\
   .\mo2_emulator.exe    # Windows
   ./mo2_emulator        # Linux/macOS
   \\\
4. **Initialize the system**:
   \\\
   CSOPESY> initialize
   \\\
5. **Start testing**:
   \\\
   CSOPESY> scheduler-test
   \\\

### Interactive Testing Script

Use the provided PowerShell script for automated testing:

\\\powershell
.\run_tests.ps1
\\\

This script provides:
- Menu-driven test case selection
- Automatic config file switching
- Step-by-step test instructions
- All 5 test cases (TC4-TC8)

---

## Commands Reference

### Core Commands

#### \initialize\
Starts the OS emulator and initializes the memory management system.
\\\
CSOPESY> initialize
\\\

#### \screen -s <process_name>\
Creates a new process with the specified name.
\\\
CSOPESY> screen -s myProcess
\\\

#### \screen -r <process_name>\
Opens the screen view of a specific process. Type \exit\ to return.
\\\
CSOPESY> screen -r myProcess
\\\

#### \screen -ls\
Lists all processes with their states and statistics.
\\\
CSOPESY> screen -ls
\\\

### Scheduler Commands

#### \scheduler-test\
**NEW in MO2** - Automated scheduler testing command.
\\\
CSOPESY> scheduler-test
\\\

#### \scheduler-start\
Starts automatic process generation.
\\\
CSOPESY> scheduler-start
\\\

#### \scheduler-stop\
Stops automatic process generation.
\\\
CSOPESY> scheduler-stop
\\\

### Memory Commands

#### \mstat\
**NEW in MO2** - Displays virtual memory statistics.
\\\
CSOPESY> vmstat
Output:
  Total Memory: 32768 KB
  Used Memory: 1024 KB
  Free Memory: 31744 KB
  Total Frames: 1024
  Used Frames: 32
  Free Frames: 992
  Total Paged In: 150
  Total Paged Out: 45
\\\

#### \process-smi\
**NEW in MO2** - Shows process and memory information similar to nvidia-smi.
\\\
CSOPESY> process-smi
Output:
  CPU Utilization: 75.00%
  Memory Usage: 1024 / 32768 KB
  Memory Utilization: 3.13%
  
  Running Processes:
  Name          Memory(KB)  Pages In  Pages Out
  process1      512         10        2
  process2      512         8         1
\\\

### Utility Commands

#### \eport-util\
Generates a CPU utilization report and saves to file.
\\\
CSOPESY> report-util
\\\

#### \clear\
Clears the screen and redraws the UI.
\\\
CSOPESY> clear
\\\

#### \exit\
Exits the OS emulator.
\\\
CSOPESY> exit
\\\

---

## Test Cases

### TC4: Generous Memory Scenario
**Goal**: Demonstrate 100% CPU utilization with minimal paging

**Configuration**:
- 4 CPUs
- 32768 KB total memory
- 32 KB per frame (1024 frames)
- 8 KB per process

**Expected Results**:
- CPU utilization: ~100%
- Most processes in "Running" state
- Low or zero paging activity
- Plenty of free memory

**Test Steps**:
\\\
1. initialize
2. scheduler-test
3. Wait 2 seconds
4. process-smi (verify ~100% CPU)
5. screen -ls (verify mostly Running)
6. vmstat (verify low paging)
7. exit
\\\

---

### TC5: High Paging Scenario
**Goal**: Demonstrate extensive paging with memory pressure

**Configuration**:
- 8 CPUs
- 1024 KB total memory
- 256 KB per frame (only 4 frames!)
- 1024 KB per process

**Expected Results**:
- High pages in count
- High pages out count
- Active paging system
- Memory pressure evident

**Test Steps**:
\\\
1. initialize
2. scheduler-test
3. Wait 10 seconds
4. scheduler-stop
5. Wait 30 seconds
6. vmstat (verify high pages in/out)
7. exit
\\\

---

### TC6: CPU Utilization Scenarios
**Goal**: Capture both 0% and 100% CPU utilization

**Configuration**:
- 1 CPU
- 4096 KB total memory
- 64 KB per frame
- 512 KB per process
- Short processes (30-45 instructions)

**Expected Results**:
- Capture 0% CPU (idle moments)
- Capture 100% CPU (when process running)
- Quick process lifecycle

**Test Steps**:
\\\
1. initialize
2. scheduler-test
3. Periodically execute screen -ls
4. Periodically execute vmstat
5. Capture screenshots at different CPU utilization levels
6. exit
\\\

---

### TC7: Moderate Utilization with Paging
**Goal**: Demonstrate >50% CPU utilization with paging activity

**Configuration**:
- 16 CPUs
- 4096 KB total memory
- 64 KB per frame
- 128-512 KB per process (variable)
- 5000 instructions per process

**Expected Results**:
- CPU utilization > 50%
- Pages in > 0
- Pages out > 0
- Mix of running and waiting processes

**Test Steps**:
\\\
1. initialize
2. scheduler-test
3. Wait 20 seconds
4. screen -ls (repeat 5 times with 5-10s intervals)
5. scheduler-stop
6. vmstat (verify >50% CPU and positive paging)
7. exit
\\\

---

### TC8: Deadlock Scenario
**Goal**: Demonstrate deadlock when process memory > total memory

**Configuration**:
- 8 CPUs
- 16384 KB total memory
- 8 KB per frame
- 32768 KB per process (EXCEEDS total memory!)

**Expected Results**:
- CPU utilization drops to 0%
- No processes can allocate memory
- All processes stuck in "Ready" state
- Deadlock condition

**Test Steps**:
\\\
1. initialize
2. scheduler-test
3. Wait 5 seconds
4. scheduler-stop
5. Periodically execute process-smi for 10 seconds
6. vmstat (verify 0% CPU - deadlock)
7. exit
\\\

---

## Architecture

### Memory Manager

The \MemoryManager\ class handles all memory-related operations:

**Key Components**:
- **PageFrame**: Represents a physical memory frame
  - \process_id\: Owner of the frame (-1 if free)
  - \page_number\: Page stored in this frame
  - \is_allocated\: Allocation status
  - \last_access\: Timestamp for LRU

- **ProcessMemoryInfo**: Tracks per-process memory details
  - \memory_required\: Total memory needed
  - \
um_pages\: Number of pages
  - \page_table\: Maps page# to frame#
  - \pages_in_memory\: Count in RAM
  - \pages_in_backing\: Count in backing store

**Key Methods**:
- \llocate_process()\: Allocates memory for a new process
- \deallocate_process()\: Frees all process memory
- \page_in()\: Brings page from backing store to memory
- \evict_page()\: LRU page replacement
- \get_stats()\: Returns memory statistics
- \get_process_info()\: Returns process memory details

### Paging Algorithm

**Page-In Simulation**:
- **FCFS**: page_in() called every 50 instructions
  - Page# = (current_line / 50) % num_pages
- **Round-Robin**: page_in() called every 10 cycles
  - Page# = (cycle_count / 10) % num_pages

**LRU Replacement**:
1. Scan all allocated frames
2. Find frame with oldest \last_access\ timestamp
3. Evict that page to backing store
4. Free the frame for new allocation

### Thread Safety

All \MemoryManager\ methods are protected by \std::mutex\ to ensure:
- Safe multi-threaded access from CPU cores
- No race conditions in frame allocation
- Consistent page table updates

---

## Known Limitations

1. **Backing Store**: Simulated in memory (no actual disk I/O)
2. **Page Size**: Must be configured before initialization (not dynamic)
3. **Memory Allocation**: Simple first-fit strategy
4. **Process Memory**: Assigned randomly within min/max range
5. **Deadlock Detection**: System does not automatically detect or resolve deadlocks

---

## File Structure

\\\
CSOPESY-MCO1-OS_Emulator-Process_Scheduler/

 Group_9_MO2_OS_Emulator.cpp     # Main source code
 mo2_emulator.exe                 # Compiled executable (Windows)
 config.txt                       # Active configuration file

 config_tc4.txt                   # Test Case 4 config
 config_tc5.txt                   # Test Case 5 config
 config_tc6.txt                   # Test Case 6 config
 config_tc7.txt                   # Test Case 7 config
 config_tc8.txt                   # Test Case 8 config

 run_tests.ps1                    # Automated test script
 MO2_CHANGES_SUMMARY.txt          # Detailed changes from MO1
 README.md                        # This file
\\\

---

## Credits

**Group 9 - CSOPESY Machine Problem 2**  
De La Salle University - Manila  
November 29, 2025

---

## Support

For issues or questions, refer to:
- \MO2_CHANGES_SUMMARY.txt\ - Comprehensive implementation details
- Source code comments - Inline documentation
- Test case configurations - Example setups

---

**End of README**
