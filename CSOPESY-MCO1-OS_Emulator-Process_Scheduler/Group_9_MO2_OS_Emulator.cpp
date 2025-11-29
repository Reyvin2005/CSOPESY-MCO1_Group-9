/*
    Course & Section: CSOPESY | S13
    Assessment: MO2 - OS Emulator - Multitasking OS with Memory Management
    Group 9 Developers: Alvarez, Ivan Antonio T.
                        Barlaan, Bahir Benjamin C.
                        Co, Joshua Benedict B.
                        Tan, Reyvin Matthew T.
    Version Date: November 29, 2025

    ═══════════════════════════════════════════════════════════════════════
    HOW TO USE THIS OS EMULATOR:
    ═══════════════════════════════════════════════════════════════════════

    COMPILATION:
    ------------
    Windows (MSVC):
        cl /EHsc /std:c++14 Group_9_MO2_OS_Emulator.cpp

    Windows (MinGW):
        g++ -std=c++14 -pthread Group_9_MO2_OS_Emulator.cpp -o mo2_emulator.exe

    Linux/Mac:
        g++ -std=c++14 -pthread Group_9_MO2_OS_Emulator.cpp -o mo2_emulator

    AVAILABLE COMMANDS:
    -------------------
    1. initialize
       - Starts the OS emulator and scheduler
       - Initializes memory management system
       - Must be run before any other commands
       - Example: initialize

    2. screen -s <process_name>
       - Creates a new process with the given name
       - Process will be added to the scheduler queue
       - Memory is allocated based on configuration
       - Example: screen -s process1

    3. screen -r <process_name>
       - Opens the screen of a specific process
       - Shows process execution details
       - Type 'exit' to return to main console
       - Example: screen -r process1

    4. screen -ls
       - Lists all processes and their current states
       - Shows: name, timestamp, core, command counters
       - Example: screen -ls

    5. scheduler-test
       - Automated scheduler testing
       - Starts the scheduler automatically
       - Useful for test case validation
       - Example: scheduler-test

    6. scheduler-start
       - Generates test processes automatically
       - Useful for testing the scheduler
       - Example: scheduler-start

    7. scheduler-stop
       - Stops automatic process generation
       - Existing processes remain in queue
       - Example: scheduler-stop

    8. report-util
       - Generates a utilization report
       - Shows CPU usage, running/finished processes
       - Saves to a text file
       - Example: report-util

    9. vmstat
       - Displays virtual memory statistics
       - Shows total/used/free memory
       - Shows frame allocation and paging activity
       - Example: vmstat

    10. process-smi
        - Shows process and memory information
        - Displays CPU utilization percentage
        - Lists memory usage per process
        - Shows paging statistics (pages in/out)
        - Example: process-smi

    11. clear
        - Clears the screen and redraws the UI
        - Example: clear

    12. exit
        - Exits the OS emulator
        - All data will be lost
        - Example: exit

    TYPICAL WORKFLOW:
    -----------------
    1. Start the program
    2. Type 'initialize' to start the OS
    3. Create processes: screen -s myProcess1
    4. View processes: screen -ls
    5. Check specific process: screen -r myProcess1
    6. Generate report: report-util
    7. Exit: exit

    SCHEDULING ALGORITHMS:
    ----------------------
    This emulator supports:
    - FCFS (First-Come-First-Served): Default, processes run to completion
    - Round-Robin: Time-sliced execution (configurable quantum)

    CONFIGURATION:
    --------------
    Create a config.txt file in the same directory with the following format:

    num-cpu 4
    scheduler fcfs
    quantum-cycles 5
    max-overall-mem 32768
    mem-per-frame 32
    min-mem-per-proc 8
    max-mem-per-proc 8
    min-ins 100
    max-ins 1000
    delays-per-exec 100
    batch-process-freq 3

    Parameters:
    - num-cpu: Number of CPU cores (default: 4)
    - scheduler: "fcfs" (First-Come-First-Served) or "rr" (Round-Robin)
    - quantum-cycles: Time quantum for round-robin (default: 5)
    - max-overall-mem: Total system memory in KB (default: 32768)
    - mem-per-frame: Memory per frame/page in KB (default: 32)
    - min-mem-per-proc: Minimum memory per process in KB (default: 8)
    - max-mem-per-proc: Maximum memory per process in KB (default: 8)
    - min-ins: Minimum instructions per process (default: 100)
    - max-ins: Maximum instructions per process (default: 1000)
    - delays-per-exec: Delay in ms per instruction (default: 100)
    - batch-process-freq: Frequency (in seconds) between automatic process creation (default: 3)

    If config.txt is not found, default values will be used.

    MEMORY MANAGEMENT (MO2):
    ------------------------
    The MO2 emulator includes:
    - Paging system with configurable frame sizes
    - LRU (Least Recently Used) page replacement algorithm
    - Backing store simulation for paged-out memory
    - Per-process page tables
    - Memory allocation/deallocation on process lifecycle
    - Page-in/page-out tracking for diagnostics
    - Virtual memory statistics via vmstat command
    - Process memory information via process-smi command

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
int BATCH_PROCESS_FREQ = 3;               // Generate process every N seconds
int DELAYS_PER_EXEC = 100;                // Delay in ms per instruction execution

// MO2: Memory Management Configuration
size_t MAX_OVERALL_MEM = 32768;           // Total system memory in KB
size_t MEM_PER_FRAME = 32;                // Memory per frame/page in KB
size_t MIN_MEM_PER_PROC = 8;              // Minimum memory per process in KB
size_t MAX_MEM_PER_PROC = 8;              // Maximum memory per process in KB

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
                if (DELAYS_PER_EXEC < 0) DELAYS_PER_EXEC = 0;  // Minimum 0ms delay
            }
            else if (key == "batch-process-freq") {
                BATCH_PROCESS_FREQ = std::stoi(value);
                if (BATCH_PROCESS_FREQ < 1) BATCH_PROCESS_FREQ = 1;
            }
            else if (key == "max-overall-mem") {
                MAX_OVERALL_MEM = std::stoull(value);
                if (MAX_OVERALL_MEM < 1) MAX_OVERALL_MEM = 1024;
            }
            else if (key == "mem-per-frame") {
                MEM_PER_FRAME = std::stoull(value);
                if (MEM_PER_FRAME < 1) MEM_PER_FRAME = 1;
            }
            else if (key == "min-mem-per-proc") {
                MIN_MEM_PER_PROC = std::stoull(value);
                if (MIN_MEM_PER_PROC < 1) MIN_MEM_PER_PROC = 1;
            }
            else if (key == "max-mem-per-proc") {
                MAX_MEM_PER_PROC = std::stoull(value);
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
// SECTION 2: MEMORY MANAGEMENT SYSTEM (MO2)
// ═══════════════════════════════════════════════════════════════════════

/*
    Memory Manager Class:
    - Implements paging system with frames
    - Manages memory allocation for processes
    - Handles page-in/page-out with backing store
    - Tracks memory usage statistics
*/
class MemoryManager {
public:
    struct PageFrame {
        int process_id;           // -1 if free
        size_t page_number;       // Page number within process
        bool is_allocated;        // Frame in use
        std::chrono::steady_clock::time_point last_access;
        
        PageFrame() : process_id(-1), page_number(0), is_allocated(false) {}
    };

    struct ProcessMemoryInfo {
        size_t memory_required;   // Total memory needed (KB)
        size_t num_pages;         // Number of pages needed
        std::vector<int> page_table;  // Maps page# to frame# (-1 if paged out)
        size_t pages_in_memory;   // Currently in RAM
        size_t pages_in_backing;  // Currently in backing store
        
        ProcessMemoryInfo() : memory_required(0), num_pages(0), pages_in_memory(0), pages_in_backing(0) {}
    };

private:
    size_t total_memory;          // Total system memory (KB)
    size_t frame_size;            // Size per frame (KB)
    size_t num_frames;            // Total frames available
    size_t num_free_frames;       // Currently free frames
    
    std::vector<PageFrame> frames;  // Physical memory frames
    std::map<int, ProcessMemoryInfo> process_memory;  // Per-process memory info
    
    mutable std::mutex mem_mutex;
    std::atomic<uint64_t> total_pages_in{0};
    std::atomic<uint64_t> total_pages_out{0};
    
    // Backing store simulation (just tracking, not actual file I/O for speed)
    std::map<std::pair<int, size_t>, bool> backing_store;  // (process_id, page#) -> exists

public:
    MemoryManager(size_t total_mem_kb, size_t frame_sz_kb)
        : total_memory(total_mem_kb), frame_size(frame_sz_kb) {
        
        num_frames = total_memory / frame_size;
        num_free_frames = num_frames;
        frames.resize(num_frames);
    }

    // Try to allocate memory for a process
    bool allocate_process(int process_id, size_t memory_kb) {
        std::lock_guard<std::mutex> lock(mem_mutex);
        
        if (process_memory.find(process_id) != process_memory.end()) {
            return false;  // Already allocated
        }
        
        ProcessMemoryInfo info;
        info.memory_required = memory_kb;
        info.num_pages = (memory_kb + frame_size - 1) / frame_size;  // Ceiling division
        info.page_table.resize(info.num_pages, -1);  // All pages start paged out
        info.pages_in_memory = 0;
        info.pages_in_backing = info.num_pages;  // All pages initially in backing store
        
        // Initialize all pages in backing store
        for (size_t i = 0; i < info.num_pages; ++i) {
            backing_store[{process_id, i}] = true;
        }
        
        process_memory[process_id] = info;
        return true;
    }

    // Deallocate all memory for a process
    void deallocate_process(int process_id) {
        std::lock_guard<std::mutex> lock(mem_mutex);
        
        auto it = process_memory.find(process_id);
        if (it == process_memory.end()) return;
        
        ProcessMemoryInfo& info = it->second;
        
        // Free all frames belonging to this process
        for (size_t page = 0; page < info.num_pages; ++page) {
            int frame_idx = info.page_table[page];
            if (frame_idx >= 0) {
                frames[frame_idx].is_allocated = false;
                frames[frame_idx].process_id = -1;
                num_free_frames++;
            }
        }
        
        // Remove from backing store
        for (size_t i = 0; i < info.num_pages; ++i) {
            backing_store.erase({process_id, i});
        }
        
        process_memory.erase(it);
    }

    // Page-in: Bring a page from backing store to memory
    // Returns true if successful, false if no free frames
    bool page_in(int process_id, size_t page_number) {
        std::lock_guard<std::mutex> lock(mem_mutex);
        
        auto it = process_memory.find(process_id);
        if (it == process_memory.end() || page_number >= it->second.num_pages) {
            return false;
        }
        
        ProcessMemoryInfo& info = it->second;
        
        // Already in memory?
        if (info.page_table[page_number] >= 0) {
            // Update access time
            int frame_idx = info.page_table[page_number];
            frames[frame_idx].last_access = std::chrono::steady_clock::now();
            return true;
        }
        
        // Need a free frame
        if (num_free_frames == 0) {
            // Try to evict a page using LRU
            if (!evict_page()) {
                return false;  // Cannot evict
            }
        }
        
        // Find first free frame
        int frame_idx = -1;
        for (size_t i = 0; i < num_frames; ++i) {
            if (!frames[i].is_allocated) {
                frame_idx = (int)i;
                break;
            }
        }
        
        if (frame_idx < 0) return false;
        
        // Allocate frame
        frames[frame_idx].is_allocated = true;
        frames[frame_idx].process_id = process_id;
        frames[frame_idx].page_number = page_number;
        frames[frame_idx].last_access = std::chrono::steady_clock::now();
        
        info.page_table[page_number] = frame_idx;
        info.pages_in_memory++;
        info.pages_in_backing--;
        num_free_frames--;
        total_pages_in++;
        
        return true;
    }

    // Page-out: Evict a page using LRU policy
    bool evict_page() {
        // Find least recently used frame
        int lru_frame = -1;
        auto oldest_time = std::chrono::steady_clock::now();
        
        for (size_t i = 0; i < num_frames; ++i) {
            if (frames[i].is_allocated && frames[i].last_access < oldest_time) {
                oldest_time = frames[i].last_access;
                lru_frame = (int)i;
            }
        }
        
        if (lru_frame < 0) return false;
        
        // Evict this frame
        PageFrame& frame = frames[lru_frame];
        int proc_id = frame.process_id;
        size_t page_num = frame.page_number;
        
        auto it = process_memory.find(proc_id);
        if (it != process_memory.end()) {
            ProcessMemoryInfo& info = it->second;
            info.page_table[page_num] = -1;  // Mark as paged out
            info.pages_in_memory--;
            info.pages_in_backing++;
            backing_store[{proc_id, page_num}] = true;
        }
        
        frame.is_allocated = false;
        frame.process_id = -1;
        num_free_frames++;
        total_pages_out++;
        
        return true;
    }

    // Get memory statistics
    void get_stats(size_t& total, size_t& used, size_t& free) const {
        std::lock_guard<std::mutex> lock(mem_mutex);
        total = total_memory;
        used = (num_frames - num_free_frames) * frame_size;
        free = num_free_frames * frame_size;
    }

    // Get process-specific memory info
    bool get_process_info(int process_id, size_t& mem_usage, size_t& num_pages_in, size_t& num_pages_out) const {
        std::lock_guard<std::mutex> lock(mem_mutex);
        auto it = process_memory.find(process_id);
        if (it == process_memory.end()) return false;
        
        const ProcessMemoryInfo& info = it->second;
        mem_usage = info.memory_required;
        num_pages_in = info.pages_in_memory;
        num_pages_out = info.pages_in_backing;
        return true;
    }

    // Get paging statistics
    void get_paging_stats(uint64_t& pages_in, uint64_t& pages_out) const {
        pages_in = total_pages_in.load();
        pages_out = total_pages_out.load();
    }

    // Check if process can fit in memory (at least minimum pages)
    bool can_allocate(size_t memory_kb) const {
        std::lock_guard<std::mutex> lock(mem_mutex);
        size_t pages_needed = (memory_kb + frame_size - 1) / frame_size;
        // For simplicity, just check if we have registered the process
        // Real check: do we have enough total memory?
        return memory_kb <= total_memory;
    }

    size_t get_num_frames() const { return num_frames; }
    size_t get_free_frames() const { 
        std::lock_guard<std::mutex> lock(mem_mutex);
        return num_free_frames; 
    }
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 3: PROCESS CLASS (Enhanced with Memory)
// ═══════════════════════════════════════════════════════════════════════

/*
    Process Class:
    - Represents a single process in the OS
    - Tracks execution state, timestamps, and progress
    - Thread-safe with internal mutex
*/
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
        FOR_END
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
    };

    struct LoopFrame {
        int start_index;       // index of first instruction inside loop body
        int end_index;         // index of FOR_END
        uint16_t remaining;    // times left to execute body
    };

    // Constructor: Creates a new process
    Process(int id, const std::string& name)
        : process_id(id),
        process_name(name),
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
    State get_state() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return state;
    }
    std::string get_timestamp() const { return timestamp; }

    // Setters (thread-safe)
    void set_core_id(int id) {
        std::lock_guard<std::mutex> lock(process_mutex);
        core_id = id;
    }
    void set_state(State s) {
        std::lock_guard<std::mutex> lock(process_mutex);
        state = s;
    }

    // Execute one instruction
    void execute_instruction() {
        std::lock_guard<std::mutex> lock(process_mutex);
        if (state == FINISHED) return;

        // Handle sleeping ticks (non-progressing, yields CPU)
        if (sleep_ticks_remaining > 0) {
            sleep_ticks_remaining--;
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
                    variables[op.var_name] = 0; // auto-declare to 0
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
            break;
        }
        case OpCode::DECLARE: {
            variables[ins.var_name] = ins.declare_value;
            current_line++;
            break;
        }
        case OpCode::ADD: {
            uint32_t a = get_value(ins.op1);
            uint32_t b = get_value(ins.op2);
            variables[ins.dest_var] = clamp16(a + b);
            current_line++;
            break;
        }
        case OpCode::SUBTRACT: {
            int32_t a = (int32_t)get_value(ins.op1);
            int32_t b = (int32_t)get_value(ins.op2);
            int32_t res = a - b;
            if (res < 0) res = 0;
            variables[ins.dest_var] = (uint16_t)res;
            current_line++;
            break;
        }
        case OpCode::SLEEP: {
            sleep_ticks_remaining = ins.sleep_ticks; // begin sleeping next cycles
            current_line++;
            break;
        }
        case OpCode::FOR_BEGIN: {
            // Find matching FOR_END by scanning forward (simple, as nesting depth is limited)
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
        return state == FINISHED || current_line >= (int)program.size();
    }

    // Get state as string
    std::string get_state_string() const {
        State s = get_state();
        switch (s) {
        case READY: return "Ready";
        case RUNNING: return "Running";
        case FINISHED: return "Finished";
        default: return "Unknown";
        }
    }

private:
    int process_id;
    std::string process_name;
    int current_line;
    int core_id;
    State state;
    std::string timestamp;
    mutable std::mutex process_mutex;

    // Instruction program and runtime state
    std::vector<Instruction> program;
    std::map<std::string, uint16_t> variables;
    std::deque<std::string> screen_logs;
    std::vector<LoopFrame> loop_stack;
    uint8_t sleep_ticks_remaining{ 0 };

    // MO2: Memory management fields
    size_t memory_required{ 0 };  // Memory requirement in KB

public:
    // MO2: Set and get memory requirement
    void set_memory_required(size_t mem_kb) {
        std::lock_guard<std::mutex> lock(process_mutex);
        memory_required = mem_kb;
    }
    
    size_t get_memory_required() const {
        std::lock_guard<std::mutex> lock(process_mutex);
        return memory_required;
    }

public:
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
    // Following spec: alternating PRINT("Value from: " +x) and ADD(x, x, [1-10])
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
        // Pattern: PRINT("Value from: " + x), ADD(x, x, [1-10]), repeat
        for (int i = 2; i < num_ins; ++i) {
            Instruction ins{};
            
            if (i % 2 == 0) {
                // Even index: PRINT("Value from: " + x)
                ins.opcode = OpCode::PRINT;
                ins.message_prefix = "Value from: ";
                ins.has_var_in_msg = true;
                ins.msg_var.is_variable = true;
                ins.msg_var.var_name = "x";
            } else {
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

};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 3: SCHEDULER CLASS
// ═══════════════════════════════════════════════════════════════════════

/*
    Scheduler Class:
    - Manages process queue and CPU cores
    - Implements FCFS or Round-Robin scheduling
    - Runs in separate thread
*/
class Scheduler {
public:
    Scheduler(int num_cores, const std::string& type, int quantum, std::shared_ptr<MemoryManager> mem_mgr = nullptr)
        : num_cores(num_cores),
        scheduler_type(type),
        quantum_cycles(quantum),
        running(false),
        next_process_id(0),
        memory_manager(mem_mgr) {

        cpu_cores.resize(num_cores, nullptr);
    }

    // Add a new process to the ready queue (MO2: with memory allocation)
    void add_process(const std::string& name, int /*instructions_unused*/) {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        auto process = std::make_shared<Process>(next_process_id++, name);
        process->build_random_program(MIN_INS, MAX_INS);
        
        // MO2: Assign random memory requirement
        size_t mem_req = MIN_MEM_PER_PROC + (rand() % (MAX_MEM_PER_PROC - MIN_MEM_PER_PROC + 1));
        process->set_memory_required(mem_req);
        
        // MO2: Allocate memory if memory manager exists
        if (memory_manager) {
            if (!memory_manager->allocate_process(process->get_id(), mem_req)) {
                // Memory allocation failed - still add to queue, will handle paging
                std::cerr << "Warning: Initial memory allocation failed for process " << name << "\\n";
            }
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
        // Worker threads are detached, so no need to join them
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
            if (pair.second->get_state() == Process::FINISHED) {
                finished_processes++;
            }
        }
    }

    // Notifies the scheduler
    void notify_all() {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        queue_cv.notify_all();
    }

private:
    // Main scheduler loop (runs in separate thread)
    void scheduler_loop() {
        while (running) {
            std::unique_lock<std::mutex> lock(scheduler_mutex);

            // Wait for processes in queue
            queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !ready_queue.empty() || !running;
                });

            if (!running) break;

            // Check for free CPU cores and assign processes
            for (int core = 0; core < num_cores; ++core) {
                // If core is free and queue has processes
                if (cpu_cores[core] == nullptr && !ready_queue.empty()) {
                    auto process = ready_queue.front();
                    ready_queue.pop();

                    cpu_cores[core] = process;
                    process->set_core_id(core);
                    process->set_state(Process::RUNNING);

                    // Launch execution thread for this process
                    std::thread t(&Scheduler::execute_process, this, process, core);
                    t.detach();  // Detach thread to avoid join issues on exit
                }
            }

            lock.unlock();
            cpu_ticks++; // simulate CPU tick increment
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Execute a process (runs in separate thread per process)
    void execute_process(std::shared_ptr<Process> process, int core) {
        // MO2: Simulate memory paging - page in first page when process starts
        if (memory_manager) {
            memory_manager->page_in(process->get_id(), 0);  // Page in first page
        }
        
        if (scheduler_type == "fcfs") {
            // FCFS: Run process to completion
            int instruction_count = 0;
            while (!process->is_finished() && running) {
                // MO2: Simulate memory access - page in periodically
                if (memory_manager && instruction_count % 50 == 0) {
                    size_t page_num = (instruction_count / 50) % ((process->get_memory_required() + MEM_PER_FRAME - 1) / MEM_PER_FRAME);
                    memory_manager->page_in(process->get_id(), page_num);
                }
                
                process->execute_instruction();
                instruction_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(DELAYS_PER_EXEC));
            }
        }
        else if (scheduler_type == "rr") {
            // Round-Robin: Execute for quantum cycles, then requeue if not finished
            int cycles_executed = 0;
            while (!process->is_finished() && running && cycles_executed < quantum_cycles) {
                // MO2: Simulate memory access
                if (memory_manager && cycles_executed % 10 == 0) {
                    int current_line = process->get_current_line();
                    size_t page_num = (current_line / 10) % ((process->get_memory_required() + MEM_PER_FRAME - 1) / MEM_PER_FRAME);
                    if (page_num < ((process->get_memory_required() + MEM_PER_FRAME - 1) / MEM_PER_FRAME)) {
                        memory_manager->page_in(process->get_id(), page_num);
                    }
                }
                
                process->execute_instruction();
                cycles_executed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(DELAYS_PER_EXEC));
            }
        }

        // If process is not finished, put it back in the queue (for RR)
        if (!process->is_finished() && running && scheduler_type == "rr") {
            std::lock_guard<std::mutex> lock(scheduler_mutex);
            process->set_state(Process::READY);
            process->set_core_id(-1);
            ready_queue.push(process);
            cpu_cores[core] = nullptr;
            queue_cv.notify_one();
        }
        else {
            // Mark finished and free the core
            process->set_state(Process::FINISHED);
            process->set_core_id(-1);
            std::lock_guard<std::mutex> lock(scheduler_mutex);
            cpu_cores[core] = nullptr;
            
            // MO2: Deallocate memory when process finishes
            if (memory_manager) {
                memory_manager->deallocate_process(process->get_id());
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

    std::mutex scheduler_mutex;
    std::condition_variable queue_cv;
    std::atomic<uint64_t> cpu_ticks{ 0 };
    std::thread scheduler_thread;
    
    // MO2: Memory Manager
    std::shared_ptr<MemoryManager> memory_manager;

public:
    // MO2: Get memory manager
    std::shared_ptr<MemoryManager> get_memory_manager() { return memory_manager; }
};

// ═══════════════════════════════════════════════════════════════════════
// SECTION 4: CONSOLE UI MANAGEMENT
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
std::shared_ptr<MemoryManager> memory_manager;  // MO2: Memory Manager
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;
std::condition_variable command_queue_cv;
std::atomic<int> global_process_counter{ 1 };
std::atomic<bool> suspend_cpu_display{ false };

// ═══════════════════════════════════════════════════════════════════════
// SECTION 5: TERMINAL CONTROL FUNCTIONS
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
// SECTION 6: UI DISPLAY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

// Display the main UI
void display_main_ui() {
    clear_screen();

    // Header
    gotoxy(1, layout.header_row);
    std::cout << Colors::BOLD << Colors::BRIGHT_BLUE
        << "========================================================================================================\n"
        << "                        CSOPESY OS EMULATOR - PROCESS SCHEDULER (MO1)                                   \n"
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
    suspend_cpu_display = true;
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
        << Colors::BRIGHT_CYAN << "Last updated: " << Colors::YELLOW << "11-5-2025\n"
        << Colors::BRIGHT_CYAN
        << "=========================================\n"
        << Colors::RESET;

    std::cout << "\nPress Enter to continue..." << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
    suspend_cpu_display = false;
}

// Update CPU utilization display
void update_cpu_display() {
    if (suspend_cpu_display) return;
    if (scheduler && system_initialized) {
        static int last_active = -1, last_running = -1, last_finished = -1;
        static uint64_t last_ticks = 0;

        int active, total, running, finished;
        scheduler->get_stats(active, total, running, finished);
        uint64_t ticks = scheduler->get_cpu_ticks();

        // Redraw if any observed value changed (including ticks)
        if (active != last_active || running != last_running || finished != last_finished || ticks != last_ticks) {
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
            printf("%sCPU Utilization: %s%d/%d cores active%s | %sRunning: %s%d%s | %sFinished: %s%d%s | %sCPU Ticks: %s%llu%s",
                Colors::BRIGHT_WHITE.c_str(),
                Colors::CYAN.c_str(), active, total, Colors::RESET.c_str(),
                Colors::BRIGHT_WHITE.c_str(), Colors::GREEN.c_str(), running, Colors::RESET.c_str(),
                Colors::BRIGHT_WHITE.c_str(), Colors::YELLOW.c_str(), finished, Colors::RESET.c_str(),
                Colors::BRIGHT_WHITE.c_str(), Colors::BRIGHT_CYAN.c_str(), (unsigned long long)ticks, Colors::RESET.c_str());
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

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -s <name>" << Colors::WHITE
        << "\n    - Creates a new process with the given name\n"
        << "    - Example: screen -s myProcess\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -r <name>" << Colors::WHITE
        << "\n    - Opens the screen of a specific process\n"
        << "    - Type 'exit' to return to main console\n"
        << "    - Example: screen -r myProcess\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  screen -ls" << Colors::WHITE
        << "\n    - Lists all processes and their states\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  scheduler-start" << Colors::WHITE
        << "\n    - Begins automatic process generation every batch-process-freq seconds\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  scheduler-stop" << Colors::WHITE
        << "\n    - Stops automatic process generation only (scheduler continues running)\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  report-util" << Colors::WHITE
        << "\n    - Generates a CPU utilization report\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  vmstat" << Colors::WHITE
        << "\n    - Displays virtual memory statistics (MO2)\n"
        << "    - Shows total/used/free memory, frames, and paging activity\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  process-smi" << Colors::WHITE
        << "\n    - Displays process memory information (MO2)\n"
        << "    - Shows CPU utilization, memory usage, and per-process memory details\n";

    std::cout << Colors::BRIGHT_YELLOW << "\n  scheduler-test" << Colors::WHITE
        << "\n    - Automated scheduler testing (MO2)\n"
        << "    - Starts automatic process generation for testing\n";

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

    if (current >= total) {
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

    std::cout << Colors::BRIGHT_WHITE << "Cores used: "
        << Colors::GREEN << active << Colors::RESET << "\n";

    std::cout << Colors::BRIGHT_WHITE << "Cores available: "
        << Colors::YELLOW << (total - active) << Colors::RESET << "\n";

    std::cout << Colors::BRIGHT_BLUE
        << "-------------------------------------------------------------\n"
        << Colors::RESET;

    // PROCESS SECTION
    auto processes = scheduler->get_all_processes();

    std::vector<std::shared_ptr<Process>> running_procs;
    std::vector<std::shared_ptr<Process>> finished_procs;

    for (auto& p : processes) {
        auto state = p->get_state();
        if (state == Process::FINISHED)
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
                << Colors::WHITE << p->get_total_commands()
                << " / " << p->get_total_commands()
                << Colors::RESET << "\n";
        }
    }

    std::cout << Colors::BRIGHT_BLUE
        << "-------------------------------------------------------------\n"
        << Colors::RESET;

    std::cout << Colors::WHITE << "Press Enter to continue..." << Colors::RESET << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
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

    file << "CSOPESY OS Emulator - CPU Utilization Report\n";
    file << "=============================================\n";
    file << "Generated: " << timestamp << "\n\n";

    file << "CPU Cores: " << total_cores << "\n";
    file << "Active Cores: " << active << "\n";
    file << "CPU Utilization: " << (total_cores > 0 ? (active * 100.0 / total_cores) : 0) << "%\n\n";
    file << "CPU Ticks: " << scheduler->get_cpu_ticks() << "\n\n";

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
        file << "  Progress: " << process->get_current_line() << "/" << process->get_total_commands() << "\n";
        file << "  Created: " << process->get_timestamp() << "\n";
    }

    file.close();

    std::cout << Colors::BRIGHT_GREEN << "Report generated: " << filename << Colors::RESET << "\n";
}


// ═══════════════════════════════════════════════════════════════════════
// SECTION 7: COMMAND HANDLERS
// ═══════════════════════════════════════════════════════════════════════

// Handle 'initialize' command
void cmd_initialize() {
    if (system_initialized) {
        std::cout << Colors::YELLOW << "System already initialized!\n" << Colors::RESET;
        return;
    }

    // MO2: Create memory manager
    memory_manager = std::make_shared<MemoryManager>(MAX_OVERALL_MEM, MEM_PER_FRAME);
    
    // Create scheduler with memory manager
    scheduler = std::make_unique<Scheduler>(NUM_CPU, SCHEDULER_TYPE, QUANTUM_CYCLES, memory_manager);
    scheduler->start();
    system_initialized = true;

    std::cout << Colors::BRIGHT_GREEN << "OS Emulator initialized successfully!\n" << Colors::RESET;
    std::cout << Colors::CYAN << "Scheduler type: " << SCHEDULER_TYPE << "\n";
    std::cout << "CPU cores: " << NUM_CPU << "\n";
    std::cout << "Memory: " << MAX_OVERALL_MEM << " KB total, " << MEM_PER_FRAME << " KB per frame\n" << Colors::RESET;
}

// Handle 'screen -s <name>' command
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

    // Create process with default program (instruction count now derived from program)
    scheduler->add_process(name, 0);
    auto p = scheduler->get_process(name);
    int instructions = p ? p->get_total_commands() : 0;
    std::cout << Colors::BRIGHT_GREEN << "Process '" << name << "' created with "
        << instructions << " instructions.\n" << Colors::RESET;
}

// Handle 'screen -r <name>' command
void cmd_screen_view(const std::string& name) {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n"
            << Colors::RESET;
        return;
    }

    auto process = scheduler->get_process(name);
    if (!process) {
        std::cout << Colors::RED << "Error: Process '" << name << "' not found!\n"
            << Colors::RESET;
        return;
    }

    if (process->is_finished()) {
        std::cout << Colors::YELLOW << "Process '" << name << "' already finished.\n"
            << "Cannot reattach. Use 'screen -ls' to view summary.\n"
            << Colors::RESET;
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

    std::lock_guard<std::mutex> lock(batch_mutex);

    if (scheduler_autorun) {
        std::cout << Colors::YELLOW << "Scheduler is already generating processes.\n" << Colors::RESET;
        return;
    }

    scheduler_autorun = true;

    std::cout << Colors::BRIGHT_YELLOW
        << "Starting continuous process generation every "
        << BATCH_PROCESS_FREQ << " seconds...\n"
        << Colors::RESET;

    batch_thread = std::thread([]() {
        uint64_t next_target = scheduler->get_cpu_ticks() + BATCH_PROCESS_FREQ;

        while (scheduler_autorun) {
            uint64_t current_ticks = scheduler->get_cpu_ticks();

            // Check if it's time to generate a new process
            if (current_ticks >= next_target) {
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

                    // Add process to scheduler
                    scheduler->add_process(name, 0);

                    // Console output (safe)
                    {
                        std::lock_guard<std::mutex> console_lock(console_mutex);
                        printf("\033[s");  // Save cursor position
                        printf("\033[%d;%dH", layout.output_start_row, 1);
                        printf("\033[K");
                        printf("%sGenerated: %s (%d instructions)%s",
                            Colors::GREEN.c_str(),
                            name.c_str(),
                            scheduler->get_process(name)->get_total_commands(),
                            Colors::RESET.c_str());
                        printf("\033[u");
                        fflush(stdout);
                    }
                }

                // Set next generation tick target
                next_target = current_ticks + BATCH_PROCESS_FREQ;
            }

            // Light sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Output stop message
        {
            std::lock_guard<std::mutex> console_lock(console_mutex);
            printf("\033[s");
            printf("\033[%d;%dH", layout.output_start_row, 1);
            printf("\033[K");
            printf("%sProcess generation stopped.%s",
                Colors::BRIGHT_YELLOW.c_str(),
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

    std::cout << Colors::BRIGHT_YELLOW
        << "Automatic process generation stopped.\n"
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

// MO2: Handle 'vmstat' command - Display virtual memory statistics
void cmd_vmstat() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n" << Colors::RESET;
        return;
    }

    if (!memory_manager) {
        std::cout << Colors::RED << "Error: Memory manager not available.\n" << Colors::RESET;
        return;
    }

    size_t total_mem, used_mem, free_mem;
    memory_manager->get_stats(total_mem, used_mem, free_mem);
    
    uint64_t pages_in, pages_out;
    memory_manager->get_paging_stats(pages_in, pages_out);
    
    size_t num_frames = memory_manager->get_num_frames();
    size_t free_frames = memory_manager->get_free_frames();
    size_t used_frames = num_frames - free_frames;
    
    std::cout << "\n" << Colors::BRIGHT_CYAN << "════════════════════════════════════════\n";
    std::cout << "         VIRTUAL MEMORY STATISTICS\n";
    std::cout << "════════════════════════════════════════\n" << Colors::RESET;
    
    std::cout << Colors::CYAN << "Total Memory:        " << Colors::WHITE << total_mem << " KB\n";
    std::cout << Colors::CYAN << "Used Memory:         " << Colors::WHITE << used_mem << " KB\n";
    std::cout << Colors::CYAN << "Free Memory:         " << Colors::WHITE << free_mem << " KB\n";
    std::cout << "\n";
    std::cout << Colors::CYAN << "Total Frames:        " << Colors::WHITE << num_frames << "\n";
    std::cout << Colors::CYAN << "Used Frames:         " << Colors::WHITE << used_frames << "\n";
    std::cout << Colors::CYAN << "Free Frames:         " << Colors::WHITE << free_frames << "\n";
    std::cout << "\n";
    std::cout << Colors::CYAN << "Num paged in:        " << Colors::WHITE << pages_in << "\n";
    std::cout << Colors::CYAN << "Num paged out:       " << Colors::WHITE << pages_out << "\n";
    std::cout << Colors::BRIGHT_CYAN << "════════════════════════════════════════\n" << Colors::RESET;
}

// MO2: Handle 'process-smi' command - Display process memory information
void cmd_process_smi() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n" << Colors::RESET;
        return;
    }

    if (!memory_manager) {
        std::cout << Colors::RED << "Error: Memory manager not available.\n" << Colors::RESET;
        return;
    }

    auto procs = scheduler->get_all_processes();
    
    // Get CPU utilization
    int active_cores, total_cores, running_procs, finished_procs;
    scheduler->get_stats(active_cores, total_cores, running_procs, finished_procs);
    double cpu_util = (total_cores > 0) ? (100.0 * active_cores / total_cores) : 0.0;
    
    size_t total_mem, used_mem, free_mem;
    memory_manager->get_stats(total_mem, used_mem, free_mem);
    
    std::cout << "\n" << Colors::BRIGHT_CYAN << "══════════════════════════════════════════════════════════════\n";
    std::cout << "                    PROCESS MEMORY INFORMATION\n";
    std::cout << "══════════════════════════════════════════════════════════════\n" << Colors::RESET;
    
    std::cout << Colors::CYAN << "CPU Utilization: " << Colors::WHITE << std::fixed << std::setprecision(1) 
              << cpu_util << "%\n";
    std::cout << Colors::CYAN << "Memory Usage:    " << Colors::WHITE << used_mem << " / " << total_mem << " KB\n";
    std::cout << Colors::CYAN << "Memory Util:     " << Colors::WHITE << std::fixed << std::setprecision(1)
              << (total_mem > 0 ? (100.0 * used_mem / total_mem) : 0.0) << "%\n";
    
    std::cout << "\n" << Colors::BRIGHT_WHITE << "Running Processes:\n" << Colors::RESET;
    std::cout << std::left << std::setw(20) << "Process" 
              << std::setw(15) << "Memory (KB)"
              << std::setw(15) << "Pages In"
              << std::setw(15) << "Pages Out" << "\n";
    std::cout << Colors::BRIGHT_CYAN << "──────────────────────────────────────────────────────────────\n" << Colors::RESET;
    
    bool has_running = false;
    for (const auto& proc : procs) {
        if (proc->get_state() != Process::FINISHED) {
            has_running = true;
            size_t mem_usage, pages_in, pages_out;
            if (memory_manager->get_process_info(proc->get_id(), mem_usage, pages_in, pages_out)) {
                std::cout << std::left << std::setw(20) << proc->get_name()
                          << std::setw(15) << mem_usage
                          << std::setw(15) << pages_in
                          << std::setw(15) << pages_out << "\n";
            }
        }
    }
    
    if (!has_running) {
        std::cout << Colors::YELLOW << "(No running processes)\n" << Colors::RESET;
    }
    
    std::cout << Colors::BRIGHT_CYAN << "══════════════════════════════════════════════════════════════\n" << Colors::RESET;
}

// MO2: Handle 'scheduler-test' command - Automated scheduler testing
void cmd_scheduler_test() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n" << Colors::RESET;
        return;
    }

    std::cout << Colors::BRIGHT_GREEN << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║          SCHEDULER TEST - Automated Execution            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n" << Colors::RESET;
    
    std::cout << Colors::CYAN << "\nStarting automated process generation...\n" << Colors::RESET;
    std::cout << Colors::YELLOW << "Press Ctrl+C to stop\n\n" << Colors::RESET;
    
    // Start the scheduler autorun
    cmd_scheduler_start();
    
    std::cout << Colors::BRIGHT_GREEN << "✓ Scheduler test initiated successfully\n" << Colors::RESET;
    std::cout << Colors::WHITE << "  - Processes will be generated every " << BATCH_PROCESS_FREQ << " second(s)\n";
    std::cout << Colors::WHITE << "  - Use 'scheduler-stop' to halt generation\n";
    std::cout << Colors::WHITE << "  - Use 'screen -ls' to view all processes\n";
    std::cout << Colors::WHITE << "  - Use 'process-smi' to view memory usage\n";
    std::cout << Colors::WHITE << "  - Use 'vmstat' to view memory statistics\n" << Colors::RESET;
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 8: COMMAND PROCESSOR
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
                << "  screen -s <name>  (create process)\n"
                << "  screen -r <name>  (view process)\n"
                << "  screen -ls        (list processes)\n" << Colors::RESET;
        }
        else if (tokens[1] == "-s" && tokens.size() >= 3) {
            cmd_screen_create(tokens[2]);
        }
        else if (tokens[1] == "-r" && tokens.size() >= 3) {
            cmd_screen_view(tokens[2]);
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
    else if (cmd == "report-util") {
        cmd_report_util();
    }
    else if (cmd == "vmstat") {
        cmd_vmstat();
    }
    else if (cmd == "process-smi") {
        cmd_process_smi();
    }
    else if (cmd == "scheduler-test") {
        cmd_scheduler_test();
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
// SECTION 9: KEYBOARD INPUT HANDLER
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
            display_main_ui();
            continue;
        }

        process_command(line);

        // Redraw prompt
        gotoxy(1, layout.prompt_row);
        std::cout << Colors::CYAN << "CSOPESY> " << Colors::RESET << std::flush;
    }
}

// CPU display update thread
void cpu_display_thread() {
    while (is_running) {
        update_cpu_display();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// ═══════════════════════════════════════════════════════════════════════
// SECTION 10: MAIN FUNCTION
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
