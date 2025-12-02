# MO2 OS Emulator – Multitasking OS with Virtual Memory  
### CSOPESY MC 02 — Group 9

## Overview
MO2 extends our MO1 OS emulator by adding a complete virtual memory system with demand paging, LRU page replacement, and backing store simulation. It also enhances process creation, instruction execution, and scheduling across multiple CPU cores. The emulator provides an interactive CLI for creating processes, monitoring memory usage, and simulating OS-level behavior.

## Features
- Virtual Memory & Paging  
  - Demand paging and page fault handling  
  - LRU page replacement when frames are full  
  - Per-process page table and memory tracking  
  - Backing store file (csopesy-backing-store.txt)  
  - 64-byte process symbol table (max 32 uint16 variables)

- CPU Scheduling  
  - FCFS and Round-Robin  
  - Multi-core simulation based on num-cpu  
  - Quantum cycles for RR  
  - Auto process generation based on config ranges  
  - Instruction delays to simulate CPU busy-waiting

- Diagnostics  
  - vmstat: detailed memory and paging stats  
  - process-smi: process and memory summary (like nvidia-smi)  
  - CPU utilization logging  
  - screen -r: interactive process display

## Installation / Compilation
### Windows (MinGW)
g++ -std=c++14 -pthread -O2 Group_9_MO2_OS_Emulator.cpp -o mo2_emulator.exe

### Windows (MSVC)
cl /EHsc /std:c++14 Group_9_MO2_OS_Emulator.cpp

### Linux / macOS
g++ -std=c++14 -pthread -O2 Group_9_MO2_OS_Emulator.cpp -o mo2_emulator  
chmod +x mo2_emulator

## Configuration (config.txt)
Example:
num-cpu 4  
scheduler rr  
quantum-cycles 5  
max-overall-mem 32768  
mem-per-frame 32  
min-mem-per-proc 128  
max-mem-per-proc 1024  
min-ins 50  
max-ins 150  
delays-per-exec 0  
batch-process-freq 1  

Notes:  
- Auto-generated process memory is random between min/max, then rounded UP to nearest power of 2, clamped between 64–65536 bytes.  
- Frame size + total memory directly control paging intensity.

## Commands
### Core
initialize  
clear  
exit  

### Process Management
screen -s <name> <memory_size>  
screen -c <name> <memory_size> "<instructions>"  
screen -r <name>  
screen -ls  

### Scheduler
scheduler-start  
scheduler-stop  

### Diagnostics
process-smi  
vmstat  
report-util  

## Architecture Summary
- Memory Manager  
  - Manages frames, page tables, backing store interaction  
  - Handles page-in/page-out, LRU eviction, and memory stats  
- Scheduler  
  - Multi-threaded CPU simulation  
  - FCFS or RR execution  
  - Requires valid pages before executing instructions  
- Processes  
  - Contain instructions, state, PC, symbol table, memory footprint, and page table


## Authors
Group 9 - CSOPESY Machine Problem 2  
Alvarez, Ivan Antonio  
Barlaan, Bahir Benjamin  
Co, Joshua Benedict  
Tan, Reyvin Matthew  
De La Salle University - Manila, 2025

