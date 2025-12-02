/*
    Course & Section: CSOPESY | S13
    Assessment: MO2 - OS Emulator - Multitasking OS
    Group 9 Developers: Alvarez, Ivan Antonio T.
                        Barlaan, Bahir Benjamin C.
                        Co, Joshua Benedict B.
                        Tan, Reyvin Matthew T.
    Version Date: December 2, 2025

    ═══════════════════════════════════════════════════════════════════════
    HOW TO USE THIS OS EMULATOR:
    ═══════════════════════════════════════════════════════════════════════

    COMPILATION:
    ------------
    Windows (MSVC):
        cl /EHsc /std:c++14 Group_9_MO2_OS_Emulator.cpp

    Windows (MinGW):
        g++ -std=c++14 -pthread Group_9_MO2_OS_Emulator.cpp -o os_emulator.exe

    Linux/Mac:
        g++ -std=c++14 -pthread Group_9_MO2_OS_Emulator.cpp -o os_emulator

    AVAILABLE COMMANDS:
    -------------------
    1. initialize
       - Starts the OS emulator and scheduler
       - Must be run before any other commands
       - Example: initialize

    2. screen -s <process_name> <memory_size>
       - Creates a new process with given name and memory allocation
       - Memory size must be power of 2 between 64-65536 bytes
       - Example: screen -s process1 256

    3. screen -c <process_name> <memory_size> "<instructions>"
       - Creates process with custom instructions
       - Example: screen -c process2 128 "DECLARE x 10; ADD x x 5; PRINT x"

    4. screen -r <process_name>
       - Opens the screen of a specific process
       - Shows process execution details
       - Type 'exit' to return to main console
       - Example: screen -r process1

    5. screen -ls
       - Lists all processes and their current states
       - Shows: name, timestamp, core, command counters, memory usage
       - Example: screen -ls

    6. scheduler-start
       - Generates test processes automatically
       - Useful for testing the scheduler
       - Example: scheduler-start

    7. scheduler-stop
       - Stops automatic process generation
       - Existing processes remain in queue
       - Example: scheduler-stop

    8. process-smi
       - Shows memory usage and process list with memory allocation
       - Similar to nvidia-smi command
       - Example: process-smi

    9. vmstat
       - Detailed memory and paging statistics
       - Example: vmstat

    10. report-util
        - Generates a utilization report
        - Shows CPU usage, running/finished processes, memory stats
        - Saves to a text file
        - Example: report-util

    11. clear
        - Clears the screen and redraws the UI
        - Example: clear

    12. exit
        - Exits the OS emulator
        - All data will be lost
        - Example: exit

    NEW INSTRUCTIONS:
    -----------------
    1. READ <var> <memory_address>
       - Reads uint16 from memory address to variable
       - Example: READ my_var 0x1000

    2. WRITE <memory_address> <value>
       - Writes uint16 value to memory address
       - Example: WRITE 0x2000 42

    TYPICAL WORKFLOW:
    -----------------
    1. Start the program
    2. Type 'initialize' to start the OS
    3. Create processes: screen -s myProcess1 256
    4. View processes: screen -ls
    5. Check memory: process-smi
    6. View details: vmstat
    7. Check specific process: screen -r myProcess1
    8. Generate report: report-util
    9. Exit: exit

    SCHEDULING ALGORITHMS:
    ----------------------
    This emulator supports:
    - FCFS (First-Come-First-Served): Default, processes run to completion
    - Round-Robin: Time-sliced execution (configurable quantum)

    MEMORY MANAGEMENT:
    ------------------
    - Demand paging with page fault handling
    - Backing store in "csopesy-backing-store.txt"
    - Memory visualization via process-smi and vmstat
    - Page replacement algorithm (FIFO)

    CONFIGURATION:
    --------------
    Create a config.txt file in the same directory with the following format:

    num-cpu 4
    scheduler rr
    quantum-cycles 5
    min-ins 100
    max-ins 1000
    delays-per-exec 100
    batch-process-freq 3
    max-overall-mem 65536
    mem-per-frame 64
    min-mem-per-proc 64

    Parameters:
    - num-cpu: Number of CPU cores (default: 4)
    - scheduler: "fcfs" (First-Come-First-Served) or "rr" (Round-Robin)
    - quantum-cycles: Time quantum for round-robin (default: 5)
    - min-ins: Minimum instructions per process (default: 100)
    - max-ins: Maximum instructions per process (default: 1000)
    - delays-per-exec: Delay in CPU TICKS per instruction (default: 100)
    - batch-process-freq: Frequency (in CPU TICKS) between automatic process creation (default: 3)
    - max-overall-mem: Maximum physical memory in bytes (default: 65536)
    - mem-per-frame: Page/frame size in bytes (default: 64)
    - min-mem-per-proc: Minimum memory per process in bytes (default: 64)

    If config.txt is not found, default values will be used.

    ═══════════════════════════════════════════════════════════════════════
*/

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <queue>
#include <vector>
#include <map>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <random>
#include <deque>
#include <bitset>
#include <unordered_map>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

// ═══════════════════════════════════════════════════════════════════════
// SECTION 1: CONFIGURATION AND CONSTANTS
// ═══════════════════════════════════════════════════════════════════════

// Color codes for terminal output
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";
    const std::string BOLD = "\033[1m";
}

// System configuration (loaded from config.txt or default values)
int NUM_CPU = 4;                          // Number of CPU cores
std::string SCHEDULER_TYPE = "fcfs";      // "fcfs" or "rr"
int QUANTUM_CYCLES = 5;                   // Time quantum for round-robin
int MIN_INS = 100;                        // Minimum instructions per process
int MAX_INS = 1000;                       // Maximum instructions per process
int BATCH_PROCESS_FREQ = 3;               // Generate process every N CPU ticks
int DELAYS_PER_EXEC = 100;                // Delay in CPU ticks per instruction execution

// MO2 Memory configuration
int MAX_OVERALL_MEM = 65536;              // Maximum physical memory in bytes
int MEM_PER_FRAME = 64;                   // Page/frame size in bytes
int MIN_MEM_PER_PROC = 64;                // Minimum memory per process
int MAX_MEM_PER_PROC = 64;                // Maximum memory per process

// Memory constants
const int SYMBOL_TABLE_SIZE = 64;         // Fixed symbol table size
const int MAX_VARIABLES = 32;             // Maximum variables per process
const uint16_t MAX_UINT16 = 65535;

// Memory address ranges
const uint32_t MIN_MEMORY_ALLOC = 64;     // 2^6
const uint32_t MAX_MEMORY_ALLOC = 65536;  // 2^16

// Function to load configuration from config.txt
void load_config() {
    std::ifstream config_file("config.txt");
    if (!config_file.is_open()) {
        std::cerr << "Warning: config.txt not found. Using default values.\n";
        std::cerr << "Looking for config.txt in current directory.\n";
        return;
    }
    std::string line;
    while (std::getline(config_file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;

        try {
            if (key == "num-cpu") {
                NUM_CPU = std::stoi(value);
                if (NUM_CPU < 1) NUM_CPU = 1;  // Minimum 1 core
            }
            else if (key == "scheduler") {
                // Remove quotes if present (e.g., "rr" -> rr)
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                SCHEDULER_TYPE = value;
            }
            else if (key == "quantum-cycles") {
                QUANTUM_CYCLES = std::stoi(value);
                if (QUANTUM_CYCLES < 1) QUANTUM_CYCLES = 1;  // Minimum 1 cycle
            }
            else if (key == "min-ins") {
                MIN_INS = std::stoi(value);
                if (MIN_INS < 1) MIN_INS = 1;  // Minimum 1 instruction
            }
            else if (key == "max-ins") {
                MAX_INS = std::stoi(value);
                if (MAX_INS < MIN_INS) MAX_INS = MIN_INS;  // Max must be >= Min
            }
            else if (key == "delays-per-exec" || key == "delay-per-exec") {
                DELAYS_PER_EXEC = std::stoi(value);
                if (DELAYS_PER_EXEC < 0) DELAYS_PER_EXEC = 0;  // Minimum 0 ticks delay
            }
            else if (key == "batch-process-freq") {
                BATCH_PROCESS_FREQ = std::stoi(value);
                if (BATCH_PROCESS_FREQ < 1) BATCH_PROCESS_FREQ = 1;
            }
            // MO2 Memory configuration
            else if (key == "max-overall-mem") {
                MAX_OVERALL_MEM = std::stoi(value);
                if (MAX_OVERALL_MEM < 64) MAX_OVERALL_MEM = 64;
            }
            else if (key == "mem-per-frame") {
                MEM_PER_FRAME = std::stoi(value);
                if (MEM_PER_FRAME < 1) MEM_PER_FRAME = 1;
            }
            else if (key == "min-mem-per-proc") {
                MIN_MEM_PER_PROC = std::stoi(value);
                if (MIN_MEM_PER_PROC < 1) MIN_MEM_PER_PROC = 1;
            }
            else if (key == "max-mem-per-proc") {
                MAX_MEM_PER_PROC = std::stoi(value);
                if (MAX_MEM_PER_PROC < MIN_MEM_PER_PROC) MAX_MEM_PER_PROC = MIN_MEM_PER_PROC;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Warning: Invalid value for '" << key << "': " << value
                << ". Using default.\n";
        }
    }

    config_file.close();
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 2: MEMORY MANAGEMENT STRUCTURES
// ═══════════════════════════════════════════════════════════════════════

// Page table entry
struct PageTableEntry {
    bool valid = false;
    int frame_number = -1;
    bool dirty = false;
    bool referenced = false;
};

// Memory frame
struct MemoryFrame {
    bool allocated = false;
    int process_id = -1;
    int page_number = -1;
    std::vector<uint8_t> data;
    uint64_t last_used = 0; // For page replacement

    MemoryFrame() : data(MEM_PER_FRAME, 0) {}
};

// Backing store entry
struct BackingStoreEntry {
    int process_id;
    int page_number;
    std::vector<uint8_t> data;
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 3: MEMORY MANAGER CLASS
// ═══════════════════════════════════════════════════════════════════════

class MemoryManager {
private:
    std::vector<MemoryFrame> physical_memory;
    std::map<int, std::vector<PageTableEntry>> page_tables; // process_id -> page table
    std::map<std::pair<int, int>, BackingStoreEntry> backing_store; // (process_id, page_number) -> data
    std::deque<int> page_replacement_queue; // FIFO for page replacement
    std::mutex memory_mutex;
    uint64_t current_tick = 0;
    std::atomic<int> pages_paged_in{ 0 };
    std::atomic<int> pages_paged_out{ 0 };

public:
    MemoryManager(int total_memory, int frame_size) {
        int num_frames = total_memory / frame_size;
        physical_memory.resize(num_frames);
    }

    // Allocates memory for a process using pre-allocation strategy.
    // Note: Unlike demand paging, we allocate physical frames immediately to ensure
    // memory stats accurately reflect actual usage. Uses FIFO page replacement when full.
    bool allocate_memory(int process_id, int memory_size) {
        std::lock_guard<std::mutex> lock(memory_mutex);

        // Validate allocation size and alignment
        if (memory_size < MIN_MEMORY_ALLOC || memory_size > MAX_MEMORY_ALLOC) {
            return false;
        }

        if ((memory_size & (memory_size - 1)) != 0) {
            return false;  // Must be power of 2
        }

        int pages_needed = (memory_size + MEM_PER_FRAME - 1) / MEM_PER_FRAME;
        if (pages_needed < 1) pages_needed = 1;
        
        // If process needs more pages than total physical frames, reject allocation
        // This creates deadlock when min-mem-per-proc > max-overall-mem (e.g., TC8)
        int total_frames = physical_memory.size();
        if (pages_needed > total_frames) {
            return false;  // Cannot allocate more than total physical memory
        }

        std::vector<PageTableEntry> page_table(pages_needed);
        
        // Pre-allocate all frames upfront
        for (int page = 0; page < pages_needed; page++) {
            
            int frame_number = -1;
            for (int i = 0; i < physical_memory.size(); i++) {
                if (!physical_memory[i].allocated) {
                    frame_number = i;
                    break;
                }
            }
            
            // Handle memory pressure with FIFO replacement
            if (frame_number == -1) {
                if (!page_replacement_queue.empty()) {
                    frame_number = page_replacement_queue.front();
                    page_replacement_queue.pop_front();
                    
                    // Evict victim frame to backing store
                    if (physical_memory[frame_number].allocated) {
                        int victim_pid = physical_memory[frame_number].process_id;
                        int victim_page = physical_memory[frame_number].page_number;
                        
                        BackingStoreEntry entry;
                        entry.process_id = victim_pid;
                        entry.page_number = victim_page;
                        entry.data = physical_memory[frame_number].data;
                        backing_store[{victim_pid, victim_page}] = entry;
                        
                        // Invalidate victim's page table entry
                        auto vit = page_tables.find(victim_pid);
                        if (vit != page_tables.end() && victim_page < vit->second.size()) {
                            vit->second[victim_page].valid = false;
                            vit->second[victim_page].frame_number = -1;
                        }
                        
                        pages_paged_out++;
                    }
                } else {
                    return false;  // OOM condition
                }
            }
            
            // Initialize frame for new process
            physical_memory[frame_number].allocated = true;
            physical_memory[frame_number].process_id = process_id;
            physical_memory[frame_number].page_number = page;
            physical_memory[frame_number].last_used = current_tick++;
            
            std::fill(physical_memory[frame_number].data.begin(), 
                      physical_memory[frame_number].data.end(), 0);
            
            // Update page table mapping
            page_table[page].valid = true;
            page_table[page].frame_number = frame_number;
            page_table[page].referenced = true;
            
            // Track for FIFO replacement
            page_replacement_queue.push_back(frame_number);
            
            pages_paged_in++;
        }
        
        page_tables[process_id] = page_table;

        return true;
    }

    // Frees all resources associated with a process
    void deallocate_memory(int process_id) {
        std::lock_guard<std::mutex> lock(memory_mutex);

        auto it = page_tables.find(process_id);
        if (it == page_tables.end()) return;

        for (int i = 0; i < it->second.size(); i++) {
            if (it->second[i].valid) {
                int frame_num = it->second[i].frame_number;
                physical_memory[frame_num].allocated = false;

                auto queue_it = std::find(page_replacement_queue.begin(),
                    page_replacement_queue.end(), frame_num);
                if (queue_it != page_replacement_queue.end()) {
                    page_replacement_queue.erase(queue_it);
                }
            }

            backing_store.erase({ process_id, i });
        }

        page_tables.erase(process_id);
    }

    // Handles page fault by loading page into physical memory
    bool handle_page_fault(int process_id, int page_number) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        current_tick++;

        auto& page_table = page_tables[process_id];
        if (page_number >= page_table.size()) return false;

        int frame_number = find_free_frame();
        if (frame_number == -1) {
            frame_number = select_victim_frame();
            if (frame_number == -1) return false;
        }

        if (physical_memory[frame_number].allocated) {
            page_out_frame(frame_number);
        }

        page_in_frame(process_id, page_number, frame_number);

        page_table[page_number].valid = true;
        page_table[page_number].frame_number = frame_number;
        page_table[page_number].referenced = true;

        physical_memory[frame_number].allocated = true;
        physical_memory[frame_number].process_id = process_id;
        physical_memory[frame_number].page_number = page_number;
        physical_memory[frame_number].last_used = current_tick;

        page_replacement_queue.push_back(frame_number);

        pages_paged_in++;
        return true;
    }

    // Reads 16-bit value from virtual address. Returns false on page fault.
    bool read_memory(int process_id, uint32_t address, uint16_t& value) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        current_tick++;

        int page_number = address / MEM_PER_FRAME;
        int offset = address % MEM_PER_FRAME;

        if (offset > MEM_PER_FRAME - 2) return false;  // uint16 requires 2 bytes

        auto it = page_tables.find(process_id);
        if (it == page_tables.end()) return false;

        auto& page_table = it->second;
        if (page_number >= page_table.size()) return false;

        if (!page_table[page_number].valid) {
            return false;  // Page fault
        }

        int frame_number = page_table[page_number].frame_number;
        if (frame_number < 0 || frame_number >= physical_memory.size()) return false;

        // Big-endian read
        value = (physical_memory[frame_number].data[offset] << 8) |
            physical_memory[frame_number].data[offset + 1];

        page_table[page_number].referenced = true;
        physical_memory[frame_number].last_used = current_tick;

        return true;
    }

    // Writes 16-bit value to virtual address. Marks page dirty for writeback.
    bool write_memory(int process_id, uint32_t address, uint16_t value) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        current_tick++;

        int page_number = address / MEM_PER_FRAME;
        int offset = address % MEM_PER_FRAME;

        if (offset > MEM_PER_FRAME - 2) return false;

        auto it = page_tables.find(process_id);
        if (it == page_tables.end()) return false;

        auto& page_table = it->second;
        if (page_number >= page_table.size()) return false;

        if (!page_table[page_number].valid) {
            return false;  // Page fault
        }

        int frame_number = page_table[page_number].frame_number;
        if (frame_number < 0 || frame_number >= physical_memory.size()) return false;

        // Big-endian write
        physical_memory[frame_number].data[offset] = (value >> 8) & 0xFF;
        physical_memory[frame_number].data[offset + 1] = value & 0xFF;

        page_table[page_number].dirty = true;
        page_table[page_number].referenced = true;
        physical_memory[frame_number].last_used = current_tick;

        return true;
    }

    // Returns current memory usage statistics
    void get_memory_stats(int& total_memory, int& used_memory, int& free_memory,
        int& total_pages, int& used_pages, int& free_pages) {
        std::lock_guard<std::mutex> lock(memory_mutex);

        total_memory = MAX_OVERALL_MEM;
        used_memory = 0;
        free_memory = 0;

        total_pages = physical_memory.size();
        used_pages = 0;
        free_pages = 0;

        for (const auto& frame : physical_memory) {
            if (frame.allocated) {
                used_memory += MEM_PER_FRAME;
                used_pages++;
            }
            else {
                free_memory += MEM_PER_FRAME;
                free_pages++;
            }
        }
    }

    // Returns paging statistics for performance monitoring
    void get_paging_stats(int& paged_in, int& paged_out) {
        paged_in = pages_paged_in.load();
        paged_out = pages_paged_out.load();
    }

    // Persists backing store state to disk for debugging
    void save_backing_store() {
        std::lock_guard<std::mutex> lock(memory_mutex);
        std::ofstream file("csopesy-backing-store.txt");
        if (!file.is_open()) return;

        // Get current time as formatted string
        time_t now = time(nullptr);
        char timestamp[80];
#if defined(_MSC_VER)
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
#elif defined(__GNUC__) && !defined(_WIN32)
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
#else
        // MinGW or other compilers
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
#endif

        file << "CSOPESY Backing Store\n";
        file << "=====================\n";
        file << "Last updated: " << timestamp << "\n";
        file << "Total entries: " << backing_store.size() << "\n\n";

        for (const auto& entry : backing_store) {
            file << "Process: " << entry.first.first
                << " Page: " << entry.first.second
                << " Size: " << entry.second.data.size() << " bytes\n";
        }

        file.close();
    }

private:
    int find_free_frame() {
        for (int i = 0; i < physical_memory.size(); i++) {
            if (!physical_memory[i].allocated) {
                return i;
            }
        }
        return -1;
    }

    // FIFO page replacement policy
    int select_victim_frame() {
        if (page_replacement_queue.empty()) return -1;

        int victim = page_replacement_queue.front();
        page_replacement_queue.pop_front();
        return victim;
    }

    // Evicts frame to backing store
    void page_out_frame(int frame_number) {
        if (!physical_memory[frame_number].allocated) return;

        int process_id = physical_memory[frame_number].process_id;
        int page_number = physical_memory[frame_number].page_number;

        auto it = page_tables.find(process_id);
        if (it != page_tables.end() && page_number < it->second.size()) {
            auto& entry = it->second[page_number];

            // Always save to backing store when evicting (for proper paging simulation)
            backing_store[{process_id, page_number}] = {
                process_id, page_number, physical_memory[frame_number].data
            };
            pages_paged_out++;

            entry.valid = false;
            entry.dirty = false;
            entry.frame_number = -1;
        }

        physical_memory[frame_number].allocated = false;
    }

    // Loads page from backing store or initializes new page
    void page_in_frame(int process_id, int page_number, int frame_number) {
        auto key = std::make_pair(process_id, page_number);
        auto it = backing_store.find(key);

        if (it != backing_store.end()) {
            physical_memory[frame_number].data = it->second.data;
            backing_store.erase(it);
        }
        else {
            std::fill(physical_memory[frame_number].data.begin(),
                physical_memory[frame_number].data.end(), 0);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 4: EXTENDED PROCESS CLASS WITH MEMORY SUPPORT
// ═══════════════════════════════════════════════════════════════════════

class Process {
public:
    enum State {
        READY,      // Waiting in queue
        RUNNING,    // Currently executing
        FINISHED    // Completed execution
    };

    // Instruction representation
    enum class OpCode {
        PRINT,
        DECLARE,
        ADD,
        SUBTRACT,
        SLEEP,
        FOR_BEGIN,
        FOR_END,
        READ,       // MO2: Memory read
        WRITE       // MO2: Memory write
    };

    struct Operand {
        bool is_variable = true;           // true: variable name, false: immediate value
        std::string var_name;              // when is_variable == true
        uint16_t imm_value = 0;            // when is_variable == false
    };

    struct Instruction {
        OpCode opcode;
        // For PRINT: message_prefix (string) and optional var operand
        std::string message_prefix;
        bool has_var_in_msg = false;
        Operand msg_var;
        // For DECLARE: var name and value
        std::string var_name;
        uint16_t declare_value = 0;
        // For arithmetic: dest, op1, op2
        std::string dest_var;
        Operand op1;
        Operand op2;
        // For SLEEP: ticks (uint8)
        uint8_t sleep_ticks = 0;
        // For FOR: repeats, matching indices resolved at runtime via stack
        uint16_t for_repeats = 0;
        // MO2: For READ/WRITE
        std::string mem_var;               // variable for READ
        uint32_t memory_address = 0;       // memory address for READ/WRITE
        uint16_t write_value = 0;          // value for WRITE
    };

    struct LoopFrame {
        int start_index;       // index of first instruction inside loop body
        int end_index;         // index of FOR_END
        uint16_t remaining;    // times left to execute body
    };

    // Extended constructor with memory allocation
    Process(int id, const std::string& name, int mem_size = 0)
        : process_id(id),
        process_name(name),
        memory_size(mem_size),
        current_line(0),
        core_id(-1),
        state(READY) {

        time_t now = time(nullptr);
        char buffer[80];

        // Use localtime_s on MSVC, localtime_r on Unix, localtime on MinGW
#if defined(_MSC_VER)
        tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", &timeinfo);
#elif defined(__GNUC__) && !defined(_WIN32)
        tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", &timeinfo);
#else
#pragma warning(push)
#pragma warning(disable: 4996)
        strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
#pragma warning(pop)
#endif

        timestamp = buffer;
    }

    // Getters (thread-safe)
    int get_id() const { return process_id; }
    std::string get_name() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return process_name;
    }
    int get_total_commands() const { std::lock_guard<std::mutex> lock(process_mutex); return (int)program.size(); }
    int get_current_line() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return current_line;
    }
    int get_core_id() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return core_id;
    }
    int get_cycles_executed() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return cycles_executed;
    }
    State get_state() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return state;
    }
    std::string get_timestamp() const { return timestamp; }
    int get_memory_size() const { return memory_size; }
    bool has_memory_violation() const { return memory_access_violation; }
    std::string get_violation_info() const {
        return violation_time + ". " + violation_address + " invalid.";
    }

    // Setters (thread-safe)
    void set_core_id(int id) {
        std::lock_guard<std::mutex> lock(process_mutex);
        core_id = id;
    }
    void set_state(State s) {
        std::lock_guard<std::mutex> lock(process_mutex);
        state = s;
    }
    void set_last_execution_tick(uint64_t tick) {
        std::lock_guard<std::mutex> lock(process_mutex);
        last_execution_tick = tick;
    }

    // Set memory violation
    void set_memory_violation(const std::string& address) {
        memory_access_violation = true;
        violation_address = address;

        // Get current time
        time_t now = time(nullptr);
        char buffer[80];
        tm timeinfo;
#if defined(_MSC_VER)
        localtime_s(&timeinfo, &now);
#else
        tm* tmp = localtime(&now);
        if (tmp) timeinfo = *tmp;
#endif
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        violation_time = buffer;
    }

    // Reset cycles executed
    void reset_cycles_executed() {
        std::lock_guard<std::mutex> lock(process_mutex);
        cycles_executed = 0;
    }

    // Determine if instruction should be executed based on delay config
    bool should_execute_instruction(uint64_t current_tick) {
        std::lock_guard<std::mutex> lock(process_mutex);

        if (DELAYS_PER_EXEC == 0) {
            // No delay - execute every tick
            return true;
        }

        // Check if enough ticks have passed since last execution
        if (current_tick >= last_execution_tick + DELAYS_PER_EXEC) {
            last_execution_tick = current_tick;
            return true;
        }

        return false;
    }

    // Execute one instruction with memory manager support
    void execute_instruction(MemoryManager* memory_manager = nullptr) {
        std::lock_guard<std::mutex> lock(process_mutex);
        if (state == FINISHED || memory_access_violation) return;

        // Handle sleeping ticks (non-progressing, yields CPU)
        if (sleep_ticks_remaining > 0) {
            sleep_ticks_remaining--;
            cycles_executed++;
            return; // do not advance current_line
        }

        if (current_line < 0 || current_line >= (int)program.size()) {
            state = FINISHED;
            return;
        }

        const Instruction& ins = program[current_line];

        auto get_value = [&](const Operand& op) -> uint16_t {
            if (op.is_variable) {
                auto it = variables.find(op.var_name);
                if (it == variables.end()) {
                    // Auto-declare to 0 if not found (within variable limit)
                    if (variables.size() < MAX_VARIABLES) {
                        variables[op.var_name] = 0;
                    }
                    return 0;
                }
                return it->second;
            }
            return op.imm_value;
            };

        auto clamp16 = [&](uint32_t v) -> uint16_t {
            if (v > 0xFFFFu) return 0xFFFFu;
            return (uint16_t)v;
            };

        switch (ins.opcode) {
        case OpCode::PRINT: {
            std::ostringstream out;
            out << ins.message_prefix;
            if (ins.has_var_in_msg) {
                out << get_value(ins.msg_var);
            }
            push_log(out.str());
            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::DECLARE: {
            if (variables.size() < MAX_VARIABLES) {
                variables[ins.var_name] = ins.declare_value;
            }
            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::ADD: {
            uint32_t a = get_value(ins.op1);
            uint32_t b = get_value(ins.op2);
            if (variables.size() < MAX_VARIABLES || variables.find(ins.dest_var) != variables.end()) {
                variables[ins.dest_var] = clamp16(a + b);
            }
            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::SUBTRACT: {
            int32_t a = (int32_t)get_value(ins.op1);
            int32_t b = (int32_t)get_value(ins.op2);
            int32_t res = a - b;
            if (res < 0) res = 0;
            if (variables.size() < MAX_VARIABLES || variables.find(ins.dest_var) != variables.end()) {
                variables[ins.dest_var] = (uint16_t)res;
            }
            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::SLEEP: {
            sleep_ticks_remaining = ins.sleep_ticks; // begin sleeping next cycles
            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::FOR_BEGIN: {
            // Find matching FOR_END by scanning forward
            int depth = 1;
            int match_idx = current_line + 1;
            while (match_idx < (int)program.size() && depth > 0) {
                if (program[match_idx].opcode == OpCode::FOR_BEGIN) depth++;
                else if (program[match_idx].opcode == OpCode::FOR_END) depth--;
                if (depth > 0) match_idx++;
            }
            if (match_idx >= (int)program.size()) {
                // Malformed, finish
                state = FINISHED;
                return;
            }
            if (ins.for_repeats == 0) {
                // Skip body entirely
                current_line = match_idx + 1;
                break;
            }
            if ((int)loop_stack.size() >= 3) {
                // Exceeds max nesting, treat as no-op body skip
                current_line = match_idx + 1;
                break;
            }
            LoopFrame frame{ current_line + 1, match_idx, ins.for_repeats };
            loop_stack.push_back(frame);
            current_line = frame.start_index;
            cycles_executed++;
            break;
        }
        case OpCode::FOR_END: {
            if (loop_stack.empty()) {
                // Malformed
                current_line++;
                break;
            }
            LoopFrame& frame = loop_stack.back();
            if (current_line != frame.end_index) {
                current_line++;
                break;
            }
            if (frame.remaining > 1) {
                frame.remaining--;
                current_line = frame.start_index;
            }
            else {
                loop_stack.pop_back();
                current_line++;
            }
            cycles_executed++;
            break;
        }
                            // MO2: Memory access instructions
        case OpCode::READ: {
            if (memory_manager == nullptr) {
                set_memory_violation("Memory manager not available");
                state = FINISHED;
                return;
            }

            uint16_t value;
            if (!read_from_memory(ins.memory_address, value, memory_manager)) {
                // Memory violation already set in read_from_memory
                state = FINISHED;
                return;
            }

            // Store in variable
            if (variables.size() < MAX_VARIABLES || variables.find(ins.mem_var) != variables.end()) {
                variables[ins.mem_var] = value;
            }

            current_line++;
            cycles_executed++;
            break;
        }
        case OpCode::WRITE: {
            if (memory_manager == nullptr) {
                set_memory_violation("Memory manager not available");
                state = FINISHED;
                return;
            }

            if (!write_to_memory(ins.memory_address, ins.write_value, memory_manager)) {
                // Memory violation already set in write_to_memory
                state = FINISHED;
                return;
            }

            current_line++;
            cycles_executed++;
            break;
        }
        }

        if (current_line >= (int)program.size()) {
            state = FINISHED;
        }
    }

    // Check if process is finished
    bool is_finished() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return state == FINISHED || current_line >= (int)program.size() || memory_access_violation;
    }

    // Get state as string
    std::string get_state_string() const {
        State s = get_state();
        if (memory_access_violation) return "Memory Violation";
        switch (s) {
        case READY: return "Ready";
        case RUNNING: return "Running";
        case FINISHED: return "Finished";
        default: return "Unknown";
        }
    }

    // Variable management for symbol table
    bool declare_variable(const std::string& name, uint16_t value) {
        if (variables.size() >= MAX_VARIABLES) return false;
        variables[name] = value;
        return true;
    }

    uint16_t get_variable(const std::string& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        return 0; // Default to 0 if not found
    }

    bool set_variable(const std::string& name, uint16_t value) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            it->second = value;
            return true;
        }
        // Auto-declare if within limit
        if (variables.size() < MAX_VARIABLES) {
            variables[name] = value;
            return true;
        }
        return false;
    }

    // Memory access methods that interface with MemoryManager
    bool read_from_memory(uint32_t address, uint16_t& value, MemoryManager* memory_manager) {
        if (address >= memory_size) {
            set_memory_violation("0x" + to_hex_string(address));
            return false;
        }

        // Handle page faults
        while (!memory_manager->read_memory(process_id, address, value)) {
            int page_number = address / MEM_PER_FRAME;
            if (!memory_manager->handle_page_fault(process_id, page_number)) {
                set_memory_violation("0x" + to_hex_string(address));
                return false;
            }
        }
        return true;
    }

    bool write_to_memory(uint32_t address, uint16_t value, MemoryManager* memory_manager) {
        if (address >= memory_size) {
            set_memory_violation("0x" + to_hex_string(address));
            return false;
        }

        // Handle page faults
        while (!memory_manager->write_memory(process_id, address, value)) {
            int page_number = address / MEM_PER_FRAME;
            if (!memory_manager->handle_page_fault(process_id, page_number)) {
                set_memory_violation("0x" + to_hex_string(address));
                return false;
            }
        }
        return true;
    }

    // Build a basic default program per spec
    void build_default_program() {
        std::lock_guard<std::mutex> lock(process_mutex);
        program.clear();
        loop_stack.clear();
        variables.clear();
        current_line = 0;
        state = READY;

        // PRINT("Hello world from <process_name>!")
        Instruction p{}; p.opcode = OpCode::PRINT; p.message_prefix = std::string("Hello world from ") + process_name + "!"; p.has_var_in_msg = false;
        program.push_back(p);

        // DECLARE(x, 0)
        Instruction d{}; d.opcode = OpCode::DECLARE; d.var_name = "x"; d.declare_value = 0; program.push_back(d);

        // ADD(x, 5, 10)
        Instruction a{}; a.opcode = OpCode::ADD; a.dest_var = "x";
        a.op1.is_variable = false; a.op1.imm_value = 5;
        a.op2.is_variable = false; a.op2.imm_value = 10;
        program.push_back(a);

        // PRINT("Value from: " + x)
        Instruction p2{}; p2.opcode = OpCode::PRINT; p2.message_prefix = "Value from: "; p2.has_var_in_msg = true;
        p2.msg_var.is_variable = true; p2.msg_var.var_name = "x";
        program.push_back(p2);

        // SLEEP(2)
        Instruction sl{}; sl.opcode = OpCode::SLEEP; sl.sleep_ticks = 2; program.push_back(sl);

        // FOR ( body: ADD(x, x, 1) ; repeats=3 )
        Instruction fb{}; fb.opcode = OpCode::FOR_BEGIN; fb.for_repeats = 3; program.push_back(fb);
        Instruction ab{}; ab.opcode = OpCode::ADD; ab.dest_var = "x";
        ab.op1.is_variable = true; ab.op1.var_name = "x";
        ab.op2.is_variable = false; ab.op2.imm_value = 1;
        program.push_back(ab);
        Instruction fe{}; fe.opcode = OpCode::FOR_END; program.push_back(fe);
    }

    // Build a random program with given instruction count range
    void build_random_program(int min_ins, int max_ins) {
        std::lock_guard<std::mutex> lock(process_mutex);
        program.clear();
        loop_stack.clear();
        variables.clear();
        current_line = 0;
        state = READY;

        int num_ins = min_ins + (rand() % (max_ins - min_ins + 1));

        // Always start with a greeting
        Instruction start{};
        start.opcode = OpCode::PRINT;
        start.message_prefix = "Hello world from " + process_name + "!";
        program.push_back(start);

        // Initialize variable x = 0 (required by spec)
        Instruction declare_x{};
        declare_x.opcode = OpCode::DECLARE;
        declare_x.var_name = "x";
        declare_x.declare_value = 0;
        program.push_back(declare_x);

        // Generate alternating PRINT and ADD instructions as per spec
        for (int i = 2; i < num_ins; ++i) {
            Instruction ins{};

            if (i % 2 == 0) {
                // Even index: PRINT("Value from: " + x)
                ins.opcode = OpCode::PRINT;
                ins.message_prefix = "Value from: ";
                ins.has_var_in_msg = true;
                ins.msg_var.is_variable = true;
                ins.msg_var.var_name = "x";
            }
            else {
                // Odd index: ADD(x, x, [1-10])
                ins.opcode = OpCode::ADD;
                ins.dest_var = "x";
                ins.op1.is_variable = true;
                ins.op1.var_name = "x";
                ins.op2.is_variable = false;
                ins.op2.imm_value = (uint16_t)(1 + rand() % 10); // Random 1-10
            }

            program.push_back(ins);
        }
    }

    // Build custom program from instruction string
    void build_custom_program(const std::string& instruction_str) {
        std::lock_guard<std::mutex> lock(process_mutex);
        program.clear();
        loop_stack.clear();
        variables.clear();
        current_line = 0;
        state = READY;

        std::vector<std::string> instructions;
        std::stringstream ss(instruction_str);
        std::string instruction;

        // Split by semicolon
        while (std::getline(ss, instruction, ';')) {
            // Trim whitespace
            instruction.erase(0, instruction.find_first_not_of(" \t\r\n"));
            instruction.erase(instruction.find_last_not_of(" \t\r\n") + 1);
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
        }

        // Validate instruction count
        if (instructions.empty() || instructions.size() > 50) {
            throw std::invalid_argument("Invalid instruction count (1-50 required)");
        }

        // Parse each instruction
        for (const auto& instr : instructions) {
            std::stringstream iss(instr);
            std::string opcode;
            iss >> opcode;

            Instruction ins{};

            if (opcode == "PRINT") {
                ins.opcode = OpCode::PRINT;
                std::string message;
                std::getline(iss, message);
                // Remove surrounding quotes if present
                if (message.front() == '"' && message.back() == '"') {
                    message = message.substr(1, message.length() - 2);
                }
                ins.message_prefix = message;
                ins.has_var_in_msg = false;
            }
            else if (opcode == "DECLARE") {
                ins.opcode = OpCode::DECLARE;
                std::string var_name;
                uint16_t value;
                iss >> var_name >> value;
                ins.var_name = var_name;
                ins.declare_value = value;
            }
            else if (opcode == "ADD") {
                ins.opcode = OpCode::ADD;
                std::string dest, op1, op2;
                iss >> dest >> op1 >> op2;
                ins.dest_var = dest;

                // Parse operands
                ins.op1 = parse_operand(op1);
                ins.op2 = parse_operand(op2);
            }
            else if (opcode == "READ") {
                ins.opcode = OpCode::READ;
                std::string var_name, addr_str;
                iss >> var_name >> addr_str;
                ins.mem_var = var_name;
                ins.memory_address = parse_hex_address(addr_str);
            }
            else if (opcode == "WRITE") {
                ins.opcode = OpCode::WRITE;
                std::string addr_str;
                uint16_t value;
                iss >> addr_str >> value;
                ins.memory_address = parse_hex_address(addr_str);
                ins.write_value = value;
            }
            else {
                throw std::invalid_argument("Unknown instruction: " + opcode);
            }

            program.push_back(ins);
        }
    }

    // Append a log line to be displayed when attached to screen
    void push_log(const std::string& s) {
        // Get current time
        time_t now = time(nullptr);
        char buffer[80];

#if defined(_MSC_VER)
        tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y   %I:%M:%S%p", &timeinfo);
#elif defined(__GNUC__) && !defined(_WIN32)
        tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buffer, sizeof(buffer), "%m/%d/%Y   %I:%M:%S%p", &timeinfo);
#else
#pragma warning(push)
#pragma warning(disable: 4996)
        strftime(buffer, sizeof(buffer), "%m/%d/%Y   %I:%M:%S%p", localtime(&now));
#pragma warning(pop)
#endif

        std::ostringstream formatted;
        formatted << "(" << buffer << ")  "
            << "Core:" << (core_id >= 0 ? std::to_string(core_id) : "N/A")
            << "  \"" << s << "\"";

        screen_logs.push_back(formatted.str());
        if (screen_logs.size() > 100) screen_logs.pop_front();
    }

    // Return all logs without clearing them
    std::vector<std::string> get_logs() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return std::vector<std::string>(screen_logs.begin(), screen_logs.end());
    }

private:
    int process_id;
    std::string process_name;
    int memory_size;
    int current_line;
    int core_id;
    State state;
    std::string timestamp;
    mutable std::mutex process_mutex;
    uint64_t last_execution_tick{ 0 };
    int cycles_executed{ 0 };

    // MO2: Memory violation tracking
    bool memory_access_violation = false;
    std::string violation_address;
    std::string violation_time;

    // Instruction program and runtime state
    std::vector<Instruction> program;
    std::map<std::string, uint16_t> variables;
    std::deque<std::string> screen_logs;
    std::vector<LoopFrame> loop_stack;
    uint8_t sleep_ticks_remaining{ 0 };

    std::string to_hex_string(uint32_t value) const {
        std::stringstream ss;
        ss << std::hex << value;
        return ss.str();
    }

    Operand parse_operand(const std::string& str) {
        Operand op;
        // Check if it's a number
        if (std::all_of(str.begin(), str.end(), ::isdigit)) {
            op.is_variable = false;
            op.imm_value = static_cast<uint16_t>(std::stoi(str));
        }
        else {
            op.is_variable = true;
            op.var_name = str;
        }
        return op;
    }

    uint32_t parse_hex_address(const std::string& str) {
        if (str.substr(0, 2) == "0x") {
            return static_cast<uint32_t>(std::stoul(str.substr(2), nullptr, 16));
        }
        return static_cast<uint32_t>(std::stoul(str));
    }
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 5: EXTENDED SCHEDULER WITH MEMORY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

class Scheduler {
public:
    Scheduler(int num_cores, const std::string& type, int quantum)
        : num_cores(num_cores),
        scheduler_type(type),
        quantum_cycles(quantum),
        running(false),
        next_process_id(0) {

        // Initialize memory manager
        memory_manager = std::make_unique<MemoryManager>(MAX_OVERALL_MEM, MEM_PER_FRAME);
        cpu_cores.resize(num_cores, nullptr);
    }

    // Add a new process to the ready queue with memory allocation
    void add_process(const std::string& name, int memory_size, const std::string& instructions = "") {
        std::lock_guard<std::mutex> lock(scheduler_mutex);

        // Validate memory size - must be between MIN_MEMORY_ALLOC and MAX_MEMORY_ALLOC bytes and power of 2
        if (memory_size < MIN_MEMORY_ALLOC || memory_size > MAX_MEMORY_ALLOC) {
            throw std::invalid_argument("Memory size must be between " + std::to_string(MIN_MEMORY_ALLOC) + " and " + std::to_string(MAX_MEMORY_ALLOC) + " bytes");
        }

        if ((memory_size & (memory_size - 1)) != 0) {
            throw std::invalid_argument("Memory size must be power of 2");
        }

        auto process = std::make_shared<Process>(next_process_id++, name, memory_size);

        // Allocate memory
        if (!memory_manager->allocate_memory(process->get_id(), memory_size)) {
            throw std::runtime_error("Memory allocation failed");
        }

        // Build program based on instructions or default
        if (instructions.empty()) {
            process->build_random_program(MIN_INS, MAX_INS);
        }
        else {
            process->build_custom_program(instructions);
        }

        ready_queue.push(process);
        all_processes[name] = process;
        queue_cv.notify_one();
    }

    // Get process by name
    std::shared_ptr<Process> get_process(const std::string& name) {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        auto it = all_processes.find(name);
        if (it != all_processes.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Get all processes
    std::vector<std::shared_ptr<Process>> get_all_processes() {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        std::vector<std::shared_ptr<Process>> processes;
        for (auto& pair : all_processes) {
            processes.push_back(pair.second);
        }
        return processes;
    }

    // Getter cpu_ticks
    uint64_t get_cpu_ticks() const {
        return cpu_ticks.load();
    }

    // Get memory manager
    MemoryManager* get_memory_manager() { return memory_manager.get(); }

    // Start the scheduler
    void start() {
        running = true;
        scheduler_thread = std::thread(&Scheduler::scheduler_loop, this);
    }

    // Stop the scheduler
    void stop() {
        running = false;
        queue_cv.notify_all();

        if (scheduler_thread.joinable()) {
            scheduler_thread.join();
        }

        // Save backing store on shutdown
        memory_manager->save_backing_store();
    }

    // Check if scheduler is running
    bool is_running() const { return running; }

    // Get CPU utilization statistics
    void get_stats(int& active_cores, int& total_cores,
        int& running_processes, int& finished_processes) {
        std::lock_guard<std::mutex> lock(scheduler_mutex);

        active_cores = 0;
        for (auto& core : cpu_cores) {
            if (core != nullptr) active_cores++;
        }

        total_cores = num_cores;
        running_processes = active_cores;
        finished_processes = 0;

        for (auto& pair : all_processes) {
            if (pair.second->get_state() == Process::FINISHED ||
                pair.second->has_memory_violation()) {
                finished_processes++;
            }
        }
    }

    // Get detailed statistics for vmstat
    void get_detailed_stats(int& total_memory, int& used_memory, int& free_memory,
        int& idle_ticks, int& active_ticks, int& total_ticks,
        int& paged_in, int& paged_out) {
        int total_pages, used_pages, free_pages;
        memory_manager->get_memory_stats(total_memory, used_memory, free_memory,
            total_pages, used_pages, free_pages);
        memory_manager->get_paging_stats(paged_in, paged_out);

        idle_ticks = idle_cpu_ticks.load();
        active_ticks = active_cpu_ticks.load();
        total_ticks = total_cpu_ticks.load();
    }

    // Notifies the scheduler
    void notify_all() {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        queue_cv.notify_all();
    }

private:
    // Main scheduler loop (runs in separate thread)
    void scheduler_loop() {
        auto last_tick_time = std::chrono::steady_clock::now();
        const auto tick_interval = std::chrono::milliseconds(10); // 10ms per tick

        while (running) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = current_time - last_tick_time;

            // Only proceed if it's time for the next tick
            if (elapsed >= tick_interval) {
                last_tick_time = current_time;

                std::unique_lock<std::mutex> lock(scheduler_mutex);

                // Update CPU tick statistics
                total_cpu_ticks++;
                int active_cores = 0;

                // Execute one cycle for all running processes
                for (int core = 0; core < num_cores; ++core) {
                    if (cpu_cores[core] != nullptr) {
                        active_cores++;
                        auto& process = cpu_cores[core];

                        // Execute instructions based on delays-per-exec
                        if (process->should_execute_instruction(cpu_ticks)) {
                            process->execute_instruction(memory_manager.get());

                            // Check if process finished or quantum expired
                            if (process->is_finished()) {
                                process->set_state(Process::FINISHED);
                                process->set_core_id(-1);
                                // Deallocate memory when process finishes
                                memory_manager->deallocate_memory(process->get_id());
                                cpu_cores[core] = nullptr;
                            }
                            else if (scheduler_type == "rr" &&
                                process->get_cycles_executed() >= quantum_cycles) {
                                // Round Robin: time slice expired, requeue
                                process->set_state(Process::READY);
                                process->set_core_id(-1);
                                process->reset_cycles_executed();
                                ready_queue.push(process);
                                cpu_cores[core] = nullptr;
                            }
                        }
                    }
                }

                // Update idle/active ticks
                if (active_cores > 0) {
                    active_cpu_ticks++;
                }
                else {
                    idle_cpu_ticks++;
                }

                // Assign processes to free cores
                for (int core = 0; core < num_cores; ++core) {
                    if (cpu_cores[core] == nullptr && !ready_queue.empty()) {
                        auto process = ready_queue.front();
                        ready_queue.pop();

                        cpu_cores[core] = process;
                        process->set_core_id(core);
                        process->set_state(Process::RUNNING);
                        process->set_last_execution_tick(cpu_ticks);
                    }
                }

                lock.unlock();
                cpu_ticks++; // Increment CPU tick counter
            }
            else {
                // Sleep briefly to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    int num_cores;
    std::string scheduler_type;
    int quantum_cycles;
    std::atomic<bool> running;
    int next_process_id;

    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> cpu_cores;
    std::map<std::string, std::shared_ptr<Process>> all_processes;
    std::unique_ptr<MemoryManager> memory_manager;

    std::mutex scheduler_mutex;
    std::condition_variable queue_cv;
    std::atomic<uint64_t> cpu_ticks{ 0 };
    std::thread scheduler_thread;

    // MO2: CPU statistics
    std::atomic<int> idle_cpu_ticks{ 0 };
    std::atomic<int> active_cpu_ticks{ 0 };
    std::atomic<int> total_cpu_ticks{ 0 };
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 6: CONSOLE UI MANAGEMENT (Existing with MO2 extensions)
// ═══════════════════════════════════════════════════════════════════════

/*
    Console Layout Structure:
    - Defines screen positions for various UI elements
    - All coordinates are 1-based (row 1, col 1 is top-left)
*/
struct ConsoleLayout {
    int screen_width = 120;
    int screen_height = 30;
    int header_row = 1;
    int status_row = 3;
    int cpu_util_row = 4;
    int help_row = 6;
    int output_start_row = 8;
    int prompt_row = 28;
};

// Global state
std::atomic<bool> is_running{ true };
std::atomic<bool> system_initialized{ false };
std::atomic<bool> scheduler_autorun{ false };
ConsoleLayout layout;
std::thread batch_thread;
std::mutex batch_mutex;
std::mutex console_mutex;
std::unique_ptr<Scheduler> scheduler;
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;
std::condition_variable command_queue_cv;
std::atomic<int> global_process_counter{ 1 };
std::atomic<bool> suspend_cpu_display{ false };

// ═══════════════════════════════════════════════════════════════════════
// SECTION 7: TERMINAL CONTROL FUNCTIONS (Existing)
// ═══════════════════════════════════════════════════════════════════════

// Enable ANSI colors on Windows
void enable_ansi_on_windows() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, mode);
#endif
}

// Control terminal cursor visibility
void set_cursor_visible(bool visible) {
    std::lock_guard<std::mutex> lock(console_mutex);
    if (visible) {
        printf("\033[?25h");
    }
    else {
        printf("\033[?25l");
    }
    fflush(stdout);
}

// Get current console size
void get_console_size(int& width, int& height) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    else {
        width = 120;
        height = 30;
    }
#else
    width = 120;
    height = 30;
#endif
}

// Move cursor to specific position (1-based coordinates)
void gotoxy(int col, int row) {
    std::lock_guard<std::mutex> lock(console_mutex);
    if (row < 1) row = 1;
    if (col < 1) col = 1;
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

// Clear the screen
void clear_screen() {
    std::lock_guard<std::mutex> lock(console_mutex);
    printf("\033[2J\033[H");
    fflush(stdout);
}

// Clear a specific line
void clear_line(int row) {
    std::lock_guard<std::mutex> lock(console_mutex);
    printf("\033[%d;%dH", row, 1);  // Move to start of line
    printf("%s", std::string(layout.screen_width, ' ').c_str());  // Clear line
    printf("\033[%d;%dH", row, 1);  // Move back to start
    fflush(stdout);
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 8: UI DISPLAY FUNCTIONS (Existing with MO2 extensions)
// ═══════════════════════════════════════════════════════════════════════

// Display the main UI
void display_main_ui() {
    clear_screen();

    // Header
    gotoxy(1, layout.header_row);
    std::cout << Colors::BOLD << Colors::BRIGHT_BLUE
        << "========================================================================================================\n"
        << "                        CSOPESY OS EMULATOR - MULTITASKING OS (MO2)                                   \n"
        << "========================================================================================================\n"
        << Colors::RESET;

    // Status
    gotoxy(1, layout.status_row + 1);
    std::cout << Colors::BRIGHT_WHITE << "System Status: " << Colors::RESET;
    if (system_initialized) {
        std::cout << Colors::BRIGHT_GREEN << "INITIALIZED" << Colors::RESET;
    }
    else {
        std::cout << Colors::YELLOW << "NOT INITIALIZED" << Colors::RESET;
    }

    // CPU Utilization
    if (scheduler && system_initialized) {
        int active, total, running, finished;
        scheduler->get_stats(active, total, running, finished);
        uint64_t ticks = scheduler->get_cpu_ticks();

        gotoxy(1, layout.cpu_util_row + 1);
        std::cout << Colors::BRIGHT_WHITE << "CPU Utilization: " << Colors::CYAN
            << active << "/" << total << " cores active" << Colors::RESET
            << " | " << Colors::BRIGHT_WHITE << "Running: " << Colors::GREEN
            << running << Colors::RESET
            << " | " << Colors::BRIGHT_WHITE << "Finished: " << Colors::YELLOW
            << finished << Colors::RESET
            << " | " << Colors::BRIGHT_WHITE << "CPU Ticks: " << Colors::BRIGHT_CYAN
            << ticks << Colors::RESET;
    }

    // Help hint
    gotoxy(1, layout.help_row + 1);
    std::cout << Colors::BRIGHT_GREEN << "Type 'help' for available commands. "
        << "Type 'initialize' to start the OS emulator." << Colors::RESET;

    // Prompt
    gotoxy(1, layout.prompt_row);
    std::cout << Colors::CYAN << "CSOPESY> " << Colors::RESET;
    set_cursor_visible(true);
}

// Display welcome screen
void display_welcome() {
    clear_screen();

    std::cout << Colors::BOLD << Colors::BRIGHT_CYAN
        << "=========================================\n"
        << "Welcome to CSOPESY Emulator!\n"
        << "\n"
        << "Developers:\n"
        << Colors::RESET << Colors::WHITE
        << "Alvarez, Ivan Antonio T. \n"
        << "Barlaan, Bahir Benjamin C.\n"
        << "Co, Joshua Benedict B.\n"
        << "Tan, Reyvin Matthew T.\n"
        << "\n"
        << Colors::BRIGHT_CYAN << "Last updated: " << Colors::YELLOW << "12-02-2025\n"
        << Colors::BRIGHT_CYAN
        << "=========================================\n"
        << Colors::RESET;

    std::cout << "\nPress Enter to continue..." << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
}

// Update CPU utilization display
void update_cpu_display() {
    if (scheduler && system_initialized) {
        static int last_active = -1, last_running = -1, last_finished = -1;
        static uint64_t last_ticks = 0;

        int active, total, running, finished;
        scheduler->get_stats(active, total, running, finished);
        uint64_t ticks = scheduler->get_cpu_ticks();

        // Only update if significant changes occurred (process counts changed OR every 5 ticks)
        bool significant_change = (active != last_active || running != last_running || 
                                   finished != last_finished || (ticks - last_ticks) >= 5);
        
        // Redraw if significant change AND display is not suspended
        if (significant_change && !suspend_cpu_display) {
            last_active = active;
            last_running = running;
            last_finished = finished;
            last_ticks = ticks;

            std::lock_guard<std::mutex> lock(console_mutex);
            printf("\033[s");  // Save cursor position
            // Clear and redraw the CPU stats line atomically
            printf("\033[%d;%dH", layout.cpu_util_row + 1, 1);
            printf("%s", std::string(layout.screen_width, ' ').c_str());
            printf("\033[%d;%dH", layout.cpu_util_row + 1, 1);

            double utilization = (total > 0) ? (active * 100.0 / total) : 0;

            printf(
                "%sCPU Utilization: %s%.0f%%%s   "
                "| %sCores Active:%s %d/%d   "
                "| %sRunning:%s %d   "
                "| %sFinished:%s %d   "
                "| %sCPU Ticks:%s %llu%s",
                Colors::BRIGHT_WHITE.c_str(),
                Colors::CYAN.c_str(), utilization, Colors::RESET.c_str(),

                Colors::BRIGHT_WHITE.c_str(), Colors::CYAN.c_str(),
                active, total,

                Colors::BRIGHT_WHITE.c_str(), Colors::GREEN.c_str(),
                running,

                Colors::BRIGHT_WHITE.c_str(), Colors::YELLOW.c_str(),
                finished,

                Colors::BRIGHT_WHITE.c_str(), Colors::BRIGHT_CYAN.c_str(),
                (unsigned long long)ticks,
                Colors::RESET.c_str()
            );
            printf("\033[u");  // Restore cursor position
            fflush(stdout);
        }
    }
}

// Display help
void display_help() {
    suspend_cpu_display = true;
    clear_screen();

    std::cout << Colors::BOLD << Colors::BRIGHT_CYAN
        << "\n==================================================================\n"
        << "                    AVAILABLE COMMANDS\n"
        << "==================================================================\n"
        << Colors::RESET;

    std::cout << Colors::BRIGHT_YELLOW << "\n  initialize" << Colors::WHITE
        << "\n    - Starts the OS emulator and scheduler\n"
        << "    - Must be run before creating processes\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -s <name> <memory_size>" << Colors::WHITE
        << "\n    - Creates a new process with given name and memory allocation\n"
        << "    - Memory size must be power of 2 between 64-65536 bytes\n"
        << "    - Example: screen -s myProcess 256\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -c <name> <memory_size> \"<instructions>\"" << Colors::WHITE
        << "\n    - Creates process with custom instructions\n"
        << "    - Example: screen -c process2 128 \"DECLARE x 10; ADD x x 5; PRINT x\"\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -r <name>" << Colors::WHITE
        << "\n    - Opens the screen of a specific process\n"
        << "    - Type 'exit' to return to main console\n"
        << "    - Example: screen -r myProcess\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -ls" << Colors::WHITE
        << "\n    - Lists all processes and their states\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  scheduler-start" << Colors::WHITE
        << "\n    - Begins automatic process generation every batch-process-freq CPU TICKS\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  scheduler-stop" << Colors::WHITE
        << "\n    - Stops automatic process generation only (scheduler continues running)\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  process-smi" << Colors::WHITE
        << "\n    - Shows memory usage and process list with memory allocation\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  vmstat" << Colors::WHITE
        << "\n    - Detailed memory and paging statistics\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  report-util" << Colors::WHITE
        << "\n    - Generates a CPU utilization report\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  clear" << Colors::WHITE
        << "\n    - Clears the screen\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  exit" << Colors::WHITE
        << "\n    - Exits the OS emulator\n";

    std::cout << Colors::BRIGHT_CYAN
        << "\n==================================================================\n"
        << Colors::RESET;

    std::cout << "\nPress Enter to continue..." << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);

    display_main_ui();
    suspend_cpu_display = false;
}

// Display process screen
void display_process_screen(std::shared_ptr<Process> process) {
    clear_screen();

    std::cout << Colors::BOLD << Colors::BRIGHT_BLUE
        << "Process: " << Colors::BRIGHT_YELLOW << process->get_name()
        << Colors::RESET << "\n";
    std::cout << Colors::BRIGHT_WHITE << "Created: " << Colors::RESET
        << process->get_timestamp() << "\n";
    std::cout << Colors::BRIGHT_WHITE << "Memory: " << Colors::RESET
        << process->get_memory_size() << " bytes\n";

    std::cout << "\nCurrent instruction line: " << process->get_current_line()
        << "\nTotal lines of instruction: " << process->get_total_commands() << "\n";

    std::cout << "\n" << Colors::BRIGHT_GREEN << "Type 'exit' to return to main console"
        << Colors::RESET << "\n\n";

    // Show process execution log
    int current = process->get_current_line();
    int total = process->get_total_commands();

    std::cout << Colors::CYAN << "Execution Log:" << Colors::RESET << "\n";
    std::cout << "-----------------------------------------------------------------------------\n";

    // Show newest logs produced by the process instructions (PRINT etc.)
    auto logs = process->get_logs();
    if (logs.empty()) {
        std::cout << Colors::WHITE << "  (no new output)" << Colors::RESET << "\n";
    }
    else {
        for (const auto& line : logs) {
            std::cout << Colors::GREEN << "  > " << Colors::WHITE << line << Colors::RESET << "\n";
        }
    }

    std::cout << "-----------------------------------------------------------------------------\n";

    if (process->has_memory_violation()) {
        std::cout << Colors::BRIGHT_RED << "\n[MEMORY VIOLATION] Process shut down: "
            << process->get_violation_info() << "\n" << Colors::RESET;
    }
    else if (current >= total) {
        std::cout << Colors::BRIGHT_GREEN << "\n[FINISHED] Process finished!\n" << Colors::RESET;
    }
    else {
        std::cout << Colors::YELLOW << "\n[RUNNING] Process running...\n" << Colors::RESET;
    }
}

// Display process list
void display_process_list() {
    if (!scheduler) {
        std::cout << Colors::RED << "Scheduler not initialized!\n" << Colors::RESET;
        return;
    }

    suspend_cpu_display = true;

    int active, total, running, finished;
    scheduler->get_stats(active, total, running, finished);
    uint64_t ticks = scheduler->get_cpu_ticks();

    // Compute CPU utilization %
    double utilization = (total > 0) ? (active * 100.0 / total) : 0.0;

    // HEADER SECTION
    std::cout << Colors::BRIGHT_WHITE << "CPU utilization: "
        << Colors::CYAN << std::fixed << std::setprecision(0)
        << utilization << "%" << Colors::RESET << "\n";

    std::cout << Colors::BRIGHT_WHITE << "Cores active: "
        << Colors::GREEN << active << "/" << total << Colors::RESET << "\n";


    std::cout << Colors::BRIGHT_BLUE
        << "-------------------------------------------------------------\n"
        << Colors::RESET;

    // PROCESS SECTION
    auto processes = scheduler->get_all_processes();

    std::vector<std::shared_ptr<Process>> running_procs;
    std::vector<std::shared_ptr<Process>> finished_procs;
    std::vector<std::shared_ptr<Process>> violation_procs;

    for (auto& p : processes) {
        if (p->has_memory_violation()) {
            violation_procs.push_back(p);
        }
        else if (p->get_state() == Process::FINISHED)
            finished_procs.push_back(p);
        else
            running_procs.push_back(p);
    }

    // Sort alphabetically by name for consistent output
    auto byName = [](auto& a, auto& b) {
        return a->get_name() < b->get_name();
        };
    std::sort(running_procs.begin(), running_procs.end(), byName);
    std::sort(finished_procs.begin(), finished_procs.end(), byName);
    std::sort(violation_procs.begin(), violation_procs.end(), byName);

    // RUNNING PROCESSES
    std::cout << Colors::BRIGHT_WHITE << "Running processes:\n" << Colors::RESET;
    if (running_procs.empty()) {
        std::cout << Colors::WHITE << "  (none)\n";
    }
    else {
        for (auto& p : running_procs) {
            std::string core_display = (p->get_core_id() == -1)
                ? "-"
                : std::to_string(p->get_core_id());

            std::cout << Colors::BRIGHT_CYAN
                << std::left << std::setw(12) << p->get_name()
                << Colors::RESET
                << " (" << Colors::YELLOW << p->get_timestamp() << Colors::RESET << ")   "
                << "Core: " << Colors::GREEN << core_display << Colors::RESET
                << "   "
                << "Mem: " << Colors::MAGENTA << p->get_memory_size() << "B" << Colors::RESET
                << "   "
                << Colors::WHITE << p->get_current_line()
                << " / " << p->get_total_commands()
                << Colors::RESET << "\n";
        }
    }

    std::cout << "\n" << Colors::BRIGHT_WHITE << "Finished processes:\n" << Colors::RESET;
    if (finished_procs.empty()) {
        std::cout << Colors::WHITE << "  (none)\n";
    }
    else {
        for (auto& p : finished_procs) {
            std::cout << Colors::BRIGHT_CYAN
                << std::left << std::setw(12) << p->get_name()
                << Colors::RESET
                << " (" << Colors::YELLOW << p->get_timestamp() << Colors::RESET << ")   "
                << Colors::GREEN << "Finished" << Colors::RESET << "   "
                << "Mem: " << Colors::MAGENTA << p->get_memory_size() << "B" << Colors::RESET
                << "   "
                << Colors::WHITE << p->get_total_commands()
                << " / " << p->get_total_commands()
                << Colors::RESET << "\n";
        }
    }

    // MEMORY VIOLATION PROCESSES
    if (!violation_procs.empty()) {
        std::cout << "\n" << Colors::BRIGHT_RED << "Memory violation processes:\n" << Colors::RESET;
        for (auto& p : violation_procs) {
            std::cout << Colors::BRIGHT_RED
                << std::left << std::setw(12) << p->get_name()
                << Colors::RESET
                << " (" << Colors::YELLOW << p->get_timestamp() << Colors::RESET << ")   "
                << Colors::BRIGHT_RED << "Memory Violation" << Colors::RESET << "   "
                << Colors::WHITE << p->get_violation_info()
                << Colors::RESET << "\n";
        }
    }

    std::cout << Colors::BRIGHT_BLUE
        << "-------------------------------------------------------------\n"
        << Colors::RESET;

    std::cout << Colors::WHITE << "Press Enter to continue..." << Colors::RESET << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
    
    // Redraw the main UI after returning from screen -ls
    display_main_ui();
    suspend_cpu_display = false;
}

// Generate utilization report
void generate_report() {
    if (!scheduler) {
        std::cout << Colors::RED << "Scheduler not initialized!\n" << Colors::RESET;
        return;
    }

    int active, total_cores, running, finished;
    scheduler->get_stats(active, total_cores, running, finished);

    auto processes = scheduler->get_all_processes();
    size_t total_processes = processes.size();

    // MO2: Get memory statistics
    int total_memory, used_memory, free_memory;
    int idle_ticks, active_ticks, total_ticks;
    int paged_in, paged_out;
    scheduler->get_detailed_stats(total_memory, used_memory, free_memory,
        idle_ticks, active_ticks, total_ticks,
        paged_in, paged_out);

    time_t now = time(nullptr);
    char timestamp[80];

    // Use localtime_s on MSVC, localtime_r on Unix, localtime on MinGW
#if defined(_MSC_VER)
    tm timeinfo;
    localtime_s(&timeinfo, &now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);
#elif defined(__GNUC__) && !defined(_WIN32)
    tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);
#else
#pragma warning(push)
#pragma warning(disable: 4996)
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
#pragma warning(pop)
#endif

    std::string filename = "csopesy-log_" + std::string(timestamp) + ".txt";
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << Colors::RED << "Error: Could not create report file!\n" << Colors::RESET;
        return;
    }

    file << "CSOPESY OS Emulator - CPU and Memory Utilization Report\n";
    file << "=======================================================\n";
    file << "Generated: " << timestamp << "\n\n";

    file << "CPU Statistics:\n";
    file << "---------------\n";
    file << "CPU Cores: " << total_cores << "\n";
    file << "Active Cores: " << active << "\n";
    file << "CPU Utilization: " << (total_cores > 0 ? (active * 100.0 / total_cores) : 0) << "%\n";
    file << "CPU Ticks: " << scheduler->get_cpu_ticks() << "\n";
    file << "Active CPU Ticks: " << active_ticks << "\n";
    file << "Idle CPU Ticks: " << idle_ticks << "\n";
    file << "Total CPU Ticks: " << total_ticks << "\n\n";

    file << "Memory Statistics:\n";
    file << "------------------\n";
    file << "Total Memory: " << total_memory << " bytes\n";
    file << "Used Memory: " << used_memory << " bytes\n";
    file << "Free Memory: " << free_memory << " bytes\n";
    file << "Memory Utilization: " << (total_memory > 0 ? (used_memory * 100.0 / total_memory) : 0) << "%\n";
    file << "Pages Paged In: " << paged_in << "\n";
    file << "Pages Paged Out: " << paged_out << "\n\n";

    file << "Process Statistics:\n";
    file << "-------------------\n";
    file << "Total Processes: " << total_processes << "\n";
    file << "Running Processes: " << running << "\n";
    file << "Finished Processes: " << finished << "\n\n";

    file << "Process Details:\n";
    file << "----------------\n";

    for (auto& process : processes) {
        file << "\nProcess: " << process->get_name() << "\n";
        file << "  ID: " << process->get_id() << "\n";
        file << "  State: " << process->get_state_string() << "\n";
        file << "  Core: " << (process->get_core_id() >= 0 ? std::to_string(process->get_core_id()) : "N/A") << "\n";
        file << "  Memory: " << process->get_memory_size() << " bytes\n";
        file << "  Progress: " << process->get_current_line() << "/" << process->get_total_commands() << "\n";
        file << "  Created: " << process->get_timestamp() << "\n";
        if (process->has_memory_violation()) {
            file << "  Memory Violation: " << process->get_violation_info() << "\n";
        }
    }

    file.close();

    std::cout << Colors::BRIGHT_GREEN << "Report generated: " << filename << Colors::RESET << "\n";
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 9: MO2 COMMAND HANDLERS
// ═══════════════════════════════════════════════════════════════════════

// Handle 'process-smi' command
void cmd_process_smi() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized.\n" << Colors::RESET;
        return;
    }

    suspend_cpu_display = true;

    int total_memory, used_memory, free_memory, total_pages, used_pages, free_pages;
    scheduler->get_memory_manager()->get_memory_stats(total_memory, used_memory, free_memory,
        total_pages, used_pages, free_pages);

    int active, total_cores, running, finished;
    scheduler->get_stats(active, total_cores, running, finished);

    double memory_util = total_memory > 0 ? (used_memory * 100.0 / total_memory) : 0;
    double cpu_util = total_cores > 0 ? (active * 100.0 / total_cores) : 0;

    // Convert bytes to KiB for display (appropriate for small memory sizes)
    double used_kib = used_memory / 1024.0;
    double total_kib = total_memory / 1024.0;

    std::cout << Colors::BRIGHT_CYAN
        << "----------------------------------------------------------------------\n"
        << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n"
        << "----------------------------------------------------------------------\n"
        << Colors::RESET;

    std::cout << Colors::BRIGHT_WHITE << "CPU-Util: " << Colors::GREEN
        << std::fixed << std::setprecision(0) << cpu_util << "%\n"
        << Colors::BRIGHT_WHITE << "Memory Usage: " << Colors::YELLOW
        << std::fixed << std::setprecision(0) << used_kib << "KiB / " << total_kib << "KiB\n"
        << Colors::BRIGHT_WHITE << "Memory Util: " << Colors::CYAN
        << std::fixed << std::setprecision(0) << memory_util << "%\n"
        << Colors::RESET;

    std::cout << Colors::BRIGHT_WHITE
        << "\nRunning processes and memory usage:\n"
        << "-----------------------------------\n"
        << Colors::RESET;

    auto processes = scheduler->get_all_processes();
    bool found_running = false;
    for (auto& process : processes) {
        if (process->get_state() != Process::FINISHED && !process->has_memory_violation()) {
            int mem_bytes = process->get_memory_size();
            std::cout << Colors::BRIGHT_GREEN << std::left << std::setw(15)
                << process->get_name()
                << Colors::YELLOW << mem_bytes << "B\n"
                << Colors::RESET;
            found_running = true;
        }
    }

    if (!found_running) {
        std::cout << Colors::WHITE << "  (no running processes)\n" << Colors::RESET;
    }

    std::cout << Colors::BRIGHT_CYAN
        << "----------------------------------------------------------------------\n"
        << Colors::RESET;

    suspend_cpu_display = false;
}

// Handle 'vmstat' command
void cmd_vmstat() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized.\n" << Colors::RESET;
        return;
    }

    suspend_cpu_display = true;

    int total_memory, used_memory, free_memory;
    int idle_ticks, active_ticks, total_ticks;
    int paged_in, paged_out;

    scheduler->get_detailed_stats(total_memory, used_memory, free_memory,
        idle_ticks, active_ticks, total_ticks,
        paged_in, paged_out);

    std::cout << Colors::BRIGHT_BLUE << "Virtual Memory Statistics\n"
        << "=======================\n" << Colors::RESET;

    std::cout << std::left << std::setw(25) << "Total memory:"
        << Colors::CYAN << total_memory << " bytes" << Colors::RESET << "\n";
    std::cout << std::left << std::setw(25) << "Active/Used memory:"
        << Colors::YELLOW << used_memory << " bytes" << Colors::RESET << "\n";
    std::cout << std::left << std::setw(25) << "Free memory:"
        << Colors::GREEN << free_memory << " bytes" << Colors::RESET << "\n";
    std::cout << std::left << std::setw(25) << "Idle CPU ticks:"
        << idle_ticks << "\n";
    std::cout << std::left << std::setw(25) << "Active CPU ticks:"
        << active_ticks << "\n";
    std::cout << std::left << std::setw(25) << "Total CPU ticks:"
        << total_ticks << "\n";
    std::cout << std::left << std::setw(25) << "Pages paged in:"
        << paged_in << "\n";
    std::cout << std::left << std::setw(25) << "Pages paged out:"
        << paged_out << "\n";

    suspend_cpu_display = false;
}

// Handle extended screen commands
void cmd_screen_create_extended(const std::vector<std::string>& tokens) {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized.\n" << Colors::RESET;
        return;
    }

    if (tokens[1] == "-s" && tokens.size() >= 4) {
        // screen -s <name> <memory_size>
        try {
            int memory_size = std::stoi(tokens[3]);
            scheduler->add_process(tokens[2], memory_size);
            std::cout << Colors::BRIGHT_GREEN << "Process '" << tokens[2]
                << "' created with " << memory_size << " bytes memory.\n" << Colors::RESET;
        }
        catch (const std::exception& e) {
            std::cout << Colors::RED << "Error: " << e.what() << "\n" << Colors::RESET;
        }
    }
    else if (tokens[1] == "-c" && tokens.size() >= 5) {
        // screen -c <name> <memory_size> "<instructions>"
        try {
            int memory_size = std::stoi(tokens[3]);
            std::string instructions = tokens[4];
            // Remove quotes if present and combine remaining tokens
            for (int i = 5; i < tokens.size(); i++) {
                instructions += " " + tokens[i];
            }
            if (instructions.front() == '"' && instructions.back() == '"') {
                instructions = instructions.substr(1, instructions.length() - 2);
            }
            scheduler->add_process(tokens[2], memory_size, instructions);
            std::cout << Colors::BRIGHT_GREEN << "Process '" << tokens[2]
                << "' created with custom instructions.\n" << Colors::RESET;
        }
        catch (const std::exception& e) {
            std::cout << Colors::RED << "Error: " << e.what() << "\n" << Colors::RESET;
        }
    }
}

// Extended screen view with memory violation handling
void cmd_screen_view_extended(const std::string& name) {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized.\n" << Colors::RESET;
        return;
    }

    auto process = scheduler->get_process(name);
    if (!process) {
        std::cout << Colors::RED << "Error: Process '" << name << "' not found!\n" << Colors::RESET;
        return;
    }

    if (process->has_memory_violation()) {
        std::cout << Colors::BRIGHT_RED << "Process '" << name
            << "' shut down due to memory access violation error that occurred at "
            << process->get_violation_info() << "\n" << Colors::RESET;
        return;
    }

    if (process->is_finished()) {
        std::cout << Colors::YELLOW << "Process '" << name << "' already finished.\n"
            << "Cannot reattach. Use 'screen -ls' to view summary.\n" << Colors::RESET;
        return;
    }

    // Enter process screen view
    suspend_cpu_display = true; // pause background UI updates to avoid overlap
    bool viewing = true;
    while (viewing && is_running) {
        display_process_screen(process);

        std::cout << "\n" << Colors::CYAN << name << "> " << Colors::RESET << std::flush;
        std::string input;
        std::getline(std::cin, input);

        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t\r\n"));
        if (!input.empty()) input.erase(input.find_last_not_of(" \t\r\n") + 1);

        if (input == "exit") {
            viewing = false;
        }
        else if (input == "process-smi") {
            display_process_screen(process);
            std::cout << "\n[Info refreshed]\n";
        }
        else if (!input.empty()) {
            std::cout << Colors::RED << "Unknown command inside process screen: "
                << input << Colors::RESET << "\n";
        }
    }

    display_main_ui();
    suspend_cpu_display = false;
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 10: EXISTING COMMAND HANDLERS (Updated for MO2)
// ═══════════════════════════════════════════════════════════════════════

// Handle 'initialize' command
void cmd_initialize() {
    if (system_initialized) {
        std::cout << Colors::YELLOW << "System already initialized!\n" << Colors::RESET;
        return;
    }

    scheduler = std::make_unique<Scheduler>(NUM_CPU, SCHEDULER_TYPE, QUANTUM_CYCLES);
    scheduler->start();
    system_initialized = true;

    std::cout << Colors::BRIGHT_GREEN << "OS Emulator initialized successfully!\n" << Colors::RESET;
    std::cout << Colors::CYAN << "Scheduler type: " << SCHEDULER_TYPE << "\n";
    std::cout << "CPU cores: " << NUM_CPU << "\n";
    std::cout << "Total memory: " << MAX_OVERALL_MEM << " bytes\n";
    std::cout << "Page size: " << MEM_PER_FRAME << " bytes\n" << Colors::RESET;
}

// Handle 'screen -s <name>' command (legacy without memory)
void cmd_screen_create(const std::string& name) {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n"
            << Colors::RESET;
        return;
    }

    // Check if process already exists
    if (scheduler->get_process(name)) {
        std::cout << Colors::RED << "Error: Process '" << name << "' already exists!\n"
            << Colors::RESET;
        return;
    }

    // Create process with default memory size (MIN_MEM_PER_PROC)
    try {
        scheduler->add_process(name, MIN_MEM_PER_PROC);
        auto p = scheduler->get_process(name);
        int instructions = p ? p->get_total_commands() : 0;
        std::cout << Colors::BRIGHT_GREEN << "Process '" << name << "' created with "
            << instructions << " instructions and " << MIN_MEM_PER_PROC << " bytes memory.\n" << Colors::RESET;
    }
    catch (const std::exception& e) {
        std::cout << Colors::RED << "Error: " << e.what() << "\n" << Colors::RESET;
    }
}

// Handle 'screen -ls' command
void cmd_screen_list() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n"
            << Colors::RESET;
        return;
    }

    display_process_list();
}

// Handle 'scheduler-start' command
void cmd_scheduler_start() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n" << Colors::RESET;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(batch_mutex);

        if (scheduler_autorun) {
            std::cout << Colors::YELLOW << "Scheduler is already generating processes.\n" << Colors::RESET;
            return;
        }

        scheduler_autorun = true;
    } // Release lock before starting thread

    std::cout << Colors::BRIGHT_YELLOW
        << "Starting continuous process generation every "
        << BATCH_PROCESS_FREQ << " CPU ticks...\n"
        << Colors::RESET;

    batch_thread = std::thread([]() {
        uint64_t next_generation_tick = scheduler->get_cpu_ticks() + BATCH_PROCESS_FREQ;

        while (scheduler_autorun && is_running) {
            uint64_t current_ticks = scheduler->get_cpu_ticks();

            // Check if it's time to generate a new process based on CPU ticks
            if (current_ticks >= next_generation_tick) {
                {
                    std::lock_guard<std::mutex> lock(batch_mutex);

                    std::ostringstream oss;
                    oss << "P" << std::setw(3) << std::setfill('0') << global_process_counter++;
                    std::string name = oss.str();

                    // Ensure unique name
                    while (scheduler->get_process(name)) {
                        oss.str("");
                        oss.clear();
                        oss << "P" << std::setw(3) << std::setfill('0') << global_process_counter++;
                        name = oss.str();
                    }

                    // Generate memory size based on config
                    // Config values (min/max-mem-per-proc) represent memory in BYTES
                    int memory_size = MIN_MEM_PER_PROC;
                    if (MAX_MEM_PER_PROC > MIN_MEM_PER_PROC) {
                        memory_size = MIN_MEM_PER_PROC + (rand() % (MAX_MEM_PER_PROC - MIN_MEM_PER_PROC + 1));
                    }
                    
                    // Ensure memory is within valid range and power of 2
                    if (memory_size < MIN_MEMORY_ALLOC) memory_size = MIN_MEMORY_ALLOC;
                    if (memory_size > MAX_MEMORY_ALLOC) memory_size = MAX_MEMORY_ALLOC;
                    
                    // Round up to nearest power of 2 if not already
                    int power = 1;
                    while (power < memory_size) power *= 2;
                    memory_size = power;

                    // Add process to scheduler with memory
                    try {
                        scheduler->add_process(name, memory_size);
                        auto new_process = scheduler->get_process(name);

                        // Console output (safe) - only if display is not suspended
                        if (!suspend_cpu_display) {
                            std::lock_guard<std::mutex> console_lock(console_mutex);
                            printf("\033[s");  // Save cursor position
                            printf("\033[%d;%dH", layout.output_start_row, 1);
                            printf("\033[K");
                            printf("%sGenerated: %s (%d instructions, %d bytes) at tick %llu%s",
                                Colors::GREEN.c_str(),
                                name.c_str(),
                                new_process ? new_process->get_total_commands() : 0,
                                memory_size,
                                (unsigned long long)current_ticks,
                                Colors::RESET.c_str());
                            printf("\033[u");
                            fflush(stdout);
                        }

                        // Set next generation tick target
                        next_generation_tick = current_ticks + BATCH_PROCESS_FREQ;
                    }
                    catch (const std::exception& e) {
                        // Silently skip failed allocations 
                        next_generation_tick = current_ticks + BATCH_PROCESS_FREQ;
                    }
                }
            }

            // Light sleep to avoid busy waiting (1ms = 100 CPU ticks at 10ms/tick)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Output stop message - only if display is not suspended
        if (!suspend_cpu_display) {
            std::lock_guard<std::mutex> console_lock(console_mutex);
            printf("\033[s");
            printf("\033[%d;%dH", layout.output_start_row, 1);
            printf("\033[K");
            printf("%sProcess generation stopped at tick %llu.%s",
                Colors::BRIGHT_YELLOW.c_str(),
                (unsigned long long)scheduler->get_cpu_ticks(),
                Colors::RESET.c_str());
            printf("\033[u");
            fflush(stdout);
        }
        });
}

// Handle 'scheduler-stop' command
void cmd_scheduler_stop() {
    if (!system_initialized) {
        std::cout << Colors::RED
            << "Error: System not initialized.\n"
            << Colors::RESET;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(batch_mutex);
        if (!scheduler_autorun) {
            std::cout << Colors::YELLOW
                << "Scheduler is not currently generating processes.\n"
                << Colors::RESET;
            return;
        }

        // Stop only the batch process generator
        scheduler_autorun = false;
    }

    // Wait for the batch generation thread to exit cleanly
    if (batch_thread.joinable()) {
        batch_thread.join();
    }

    // Save backing store when stopping (for TC7 requirement)
    scheduler->get_memory_manager()->save_backing_store();

    std::cout << Colors::BRIGHT_YELLOW
        << "Automatic process generation stopped.\n"
        << "Backing store saved to csopesy-backing-store.txt\n"
        << Colors::RESET;
}

// Handle 'report-util' command
void cmd_report_util() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n" << Colors::RESET;
        return;
    }

    auto procs = scheduler->get_all_processes();
    if (procs.empty()) {
        std::cout << Colors::YELLOW << "No processes available to report.\n" << Colors::RESET;
        return;
    }

    generate_report();
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 11: COMMAND PROCESSOR (Updated for MO2)
// ═══════════════════════════════════════════════════════════════════════

// Parse and execute command
void process_command(const std::string& input) {
    if (input.empty()) return;

    // Split command into tokens
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    // Clear output area
    for (int i = layout.output_start_row; i < layout.prompt_row - 1; i++) {
        clear_line(i);
    }

    gotoxy(1, layout.output_start_row);

    // Execute command
    if (cmd == "help") {
        display_help();
    }
    else if (cmd == "initialize") {
        cmd_initialize();
    }
    else if (cmd == "screen") {
        if (tokens.size() < 2) {
            std::cout << Colors::RED << "Error: Invalid screen command. Usage:\n"
                << "  screen -s <name> <memory_size>  (create process with memory)\n"
                << "  screen -c <name> <memory_size> \"<instructions>\"  (create with custom instructions)\n"
                << "  screen -r <name>  (view process)\n"
                << "  screen -ls        (list processes)\n" << Colors::RESET;
        }
        else if (tokens[1] == "-s") {
            if (tokens.size() >= 4) {
                cmd_screen_create_extended(tokens);
            }
            else {
                // Legacy support: screen -s <name> without memory
                if (tokens.size() >= 3) {
                    cmd_screen_create(tokens[2]);
                }
                else {
                    std::cout << Colors::RED << "Error: screen -s requires process name and memory size\n" << Colors::RESET;
                }
            }
        }
        else if (tokens[1] == "-c" && tokens.size() >= 5) {
            cmd_screen_create_extended(tokens);
        }
        else if (tokens[1] == "-r" && tokens.size() >= 3) {
            cmd_screen_view_extended(tokens[2]);
        }
        else if (tokens[1] == "-ls") {
            cmd_screen_list();
        }
        else {
            std::cout << Colors::RED << "Error: Unknown screen option '" << tokens[1] << "'\n"
                << Colors::RESET;
        }
    }
    else if (cmd == "scheduler-start") {
        cmd_scheduler_start();
    }
    else if (cmd == "scheduler-stop") {
        cmd_scheduler_stop();
    }
    else if (cmd == "process-smi") {
        cmd_process_smi();
    }
    else if (cmd == "vmstat") {
        cmd_vmstat();
    }
    else if (cmd == "report-util") {
        cmd_report_util();
    }
    else if (cmd == "clear") {
        display_main_ui();
    }
    else if (cmd == "exit") {
        is_running = false;
        scheduler_autorun = false;

        if (scheduler) scheduler->notify_all();
    }
    else {
        std::cout << Colors::RED << "Unknown command: " << cmd << "\n"
            << "Type 'help' for available commands." << Colors::RESET;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 12: KEYBOARD INPUT HANDLER (Existing)
// ═══════════════════════════════════════════════════════════════════════

// Keyboard input thread
void keyboard_handler_thread() {
    std::string line;
    while (is_running) {
        // Position cursor right after the prompt and flush
        gotoxy(10, layout.prompt_row);  // Position after "CSOPESY> "
        std::cout << std::flush;

        if (!std::getline(std::cin, line)) {
            is_running = false;
            break;
        }

        // Suspend CPU display while processing command to prevent overlap
        suspend_cpu_display = true;

        // Clear the input area (from column 10 onwards) to remove the typed command
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            printf("\033[%d;%dH", layout.prompt_row, 10);  // Move to input start (don't use gotoxy - it locks mutex!)
            printf("%s", std::string(layout.screen_width - 10, ' ').c_str());  // Clear from here to end
            fflush(stdout);
        }

        // Trim input
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (!line.empty()) {
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
        }

        if (line.empty()) {
            suspend_cpu_display = false;
            display_main_ui();
            continue;
        }

        process_command(line);

        // Re-enable CPU display after command completes
        suspend_cpu_display = false;

        // Redraw prompt
        gotoxy(1, layout.prompt_row);
        std::cout << Colors::CYAN << "CSOPESY> " << Colors::RESET << std::flush;
    }
}

// CPU display update thread
void cpu_display_thread() {
    while (is_running) {
        update_cpu_display();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 13: MAIN FUNCTION
// ═══════════════════════════════════════════════════════════════════════

int main() {
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // Load configuration from file
    load_config();

    // Initialize terminal
    enable_ansi_on_windows();
    get_console_size(layout.screen_width, layout.screen_height);

    // Display welcome screen
    display_welcome();

    // Display UI
    display_main_ui();

    // Start CPU display update thread
    std::thread cpu_thread(cpu_display_thread);

    // Run keyboard handler in main thread
    keyboard_handler_thread();

    // ==== SHUTDOWN SEQUENCE ====
    is_running = false;
    scheduler_autorun = false;

    // Stop scheduler safely
    if (scheduler) {
        scheduler->stop();
    }

    // Stop batch process generator
    if (batch_thread.joinable()) {
        batch_thread.join();
    }

    // Stop CPU display updater
    if (cpu_thread.joinable()) {
        cpu_thread.join();
    }

    // ==== CLEAN EXIT ====
    clear_screen();
    std::cout << Colors::BRIGHT_RED
        << "CSOPESY OS Emulator shutting down...\n"
        << Colors::RESET;
    std::cout << Colors::BRIGHT_YELLOW
        << "Thank you for using our system!\n"
        << Colors::RESET;

    return 0;
}
