/*
    Course & Section: CSOPESY | S13
    Assessment: MO1 - OS Emulator - Process Scheduler
    Group 9 Developers: Alvarez, Ivan Antonio T.
                        Barlaan, Bahir Benjamin C.
                        Co, Joshua Benedict B.
                        Tan, Reyvin Matthew T.
    Version Date: October 31, 2025

    ═══════════════════════════════════════════════════════════════════════
    HOW TO USE THIS OS EMULATOR:
    ═══════════════════════════════════════════════════════════════════════

    COMPILATION:
    ------------
    Windows (MSVC):
        cl /EHsc /std:c++14 Group_9_MO1_OS_Emulator.cpp

    Windows (MinGW):
        g++ -std=c++14 -pthread Group_9_MO1_OS_Emulator.cpp -o os_emulator.exe

    Linux/Mac:
        g++ -std=c++14 -pthread Group_9_MO1_OS_Emulator.cpp -o os_emulator

    AVAILABLE COMMANDS:
    -------------------
    1. initialize
       - Starts the OS emulator and scheduler
       - Must be run before any other commands
       - Example: initialize

    2. screen -s <process_name>
       - Creates a new process with the given name
       - Process will be added to the scheduler queue
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

    5. scheduler-start
       - Generates test processes automatically
       - Useful for testing the scheduler
       - Example: scheduler-start

    6. scheduler-stop
       - Stops the scheduler from running
       - Existing processes remain in queue
       - Example: scheduler-stop

    7. report-util
       - Generates a utilization report
       - Shows CPU usage, running/finished processes
       - Saves to a text file
       - Example: report-util

    8. clear
       - Clears the screen and redraws the UI
       - Example: clear

    9. exit
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
    min-ins 100
    max-ins 1000
    delays-per-exec 100
    batch-process-freq 3

    Parameters:
    - num-cpu: Number of CPU cores (default: 4)
    - scheduler: "fcfs" (First-Come-First-Served) or "rr" (Round-Robin)
    - quantum-cycles: Time quantum for round-robin (default: 5)
    - min-ins: Minimum instructions per process (default: 100)
    - max-ins: Maximum instructions per process (default: 1000)
    - delays-per-exec: Delay in ms per instruction (default: 100)
    - batch-process-freq: Frequency (in seconds) between automatic process creation (default: 3)

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

// Function to load configuration from config.txt
void load_config() {
    std::ifstream config_file("config.txt");
    if (!config_file.is_open()) {
        std::cerr << "Warning: config.txt not found. Using default values.\n";
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
            else if (key == "delays-per-exec") {
                DELAYS_PER_EXEC = std::stoi(value);
                if (DELAYS_PER_EXEC < 0) DELAYS_PER_EXEC = 0;  // Minimum 0ms delay
            }
            else if (key == "batch-process-freq") {
                BATCH_PROCESS_FREQ = std::stoi(value);
                if (BATCH_PROCESS_FREQ < 1) BATCH_PROCESS_FREQ = 1;
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
// SECTION 2: PROCESS CLASS
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
            } else {
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
        Instruction a{}; a.opcode = OpCode::ADD; a.dest_var = "x"; a.op1 = Operand{ false, "", 5 }; a.op2 = Operand{ false, "", 10 }; program.push_back(a);

        // PRINT("Value from: " + x)
        Instruction p2{}; p2.opcode = OpCode::PRINT; p2.message_prefix = "Value from: "; p2.has_var_in_msg = true; p2.msg_var = Operand{ true, "x", 0 }; program.push_back(p2);

        // SLEEP(2)
        Instruction sl{}; sl.opcode = OpCode::SLEEP; sl.sleep_ticks = 2; program.push_back(sl);

        // FOR ( body: ADD(x, x, 1) ; repeats=3 )
        Instruction fb{}; fb.opcode = OpCode::FOR_BEGIN; fb.for_repeats = 3; program.push_back(fb);
        Instruction ab{}; ab.opcode = OpCode::ADD; ab.dest_var = "x"; ab.op1 = Operand{ true, "x", 0 }; ab.op2 = Operand{ false, "", 1 }; program.push_back(ab);
        Instruction fe{}; fe.opcode = OpCode::FOR_END; program.push_back(fe);
    }

    // Append a log line to be displayed when attached to screen
    void push_log(const std::string& s) {
        screen_logs.push_back(s);
        // keep only recent 100 lines
        if (screen_logs.size() > 100) screen_logs.pop_front();
    }

    // Drain logs for UI display
    std::vector<std::string> drain_logs() {
        std::vector<std::string> out(screen_logs.begin(), screen_logs.end());
        screen_logs.clear();
        return out;
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
    Scheduler(int num_cores, const std::string& type, int quantum)
        : num_cores(num_cores),
        scheduler_type(type),
        quantum_cycles(quantum),
        running(false),
        next_process_id(0) {

        cpu_cores.resize(num_cores, nullptr);
    }

    // Add a new process to the ready queue
    void add_process(const std::string& name, int /*instructions_unused*/) {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        auto process = std::make_shared<Process>(next_process_id++, name);
        process->build_default_program();
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
                    worker_threads.push_back(std::move(t));
                }
            }

            lock.unlock();
            cpu_ticks++; // simulate CPU tick increment
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // Execute a process (runs in separate thread per process)
    void execute_process(std::shared_ptr<Process> process, int core) {
        if (scheduler_type == "fcfs") {
            // FCFS: Run process to completion
            while (!process->is_finished() && running) {
                process->execute_instruction();
                std::this_thread::sleep_for(std::chrono::milliseconds(DELAYS_PER_EXEC));
            }
        }
        else if (scheduler_type == "rr") {
            // Round-Robin: Execute for quantum cycles, then requeue if not finished
            int cycles_executed = 0;
            while (!process->is_finished() && running && cycles_executed < quantum_cycles) {
                process->execute_instruction();
                cycles_executed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(DELAYS_PER_EXEC));
            }

            // If process is not finished, put it back in the queue
            if (!process->is_finished() && running) {
                std::lock_guard<std::mutex> lock(scheduler_mutex);
                process->set_state(Process::READY);
                process->set_core_id(-1);
                ready_queue.push(process);
                cpu_cores[core] = nullptr;
                queue_cv.notify_one();
                return; // Don't mark as finished yet
            }
        }

        // Mark as finished and free the core
        process->set_state(Process::FINISHED);
        process->set_core_id(-1);

        std::lock_guard<std::mutex> lock(scheduler_mutex);
        cpu_cores[core] = nullptr;
    }

    int num_cores;
    std::string scheduler_type;
    int quantum_cycles;
    std::atomic<bool> running;
    int next_process_id;

    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> cpu_cores;
    std::vector<std::thread> worker_threads;
    std::map<std::string, std::shared_ptr<Process>> all_processes;

    std::mutex scheduler_mutex;
    std::condition_variable queue_cv;
    std::atomic<uint64_t> cpu_ticks{ 0 };
    std::thread scheduler_thread;
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
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;
std::condition_variable command_queue_cv;

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
}

// Update CPU utilization display
void update_cpu_display() {
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

            // Save cursor position
            {
                std::lock_guard<std::mutex> lock(console_mutex);
                printf("\033[s");  // Save cursor position
                fflush(stdout);
            }

            clear_line(layout.cpu_util_row + 1);
            gotoxy(1, layout.cpu_util_row + 1);
            std::cout << Colors::BRIGHT_WHITE << "CPU Utilization: " << Colors::CYAN
                << active << "/" << total << " cores active" << Colors::RESET
                << " | " << Colors::BRIGHT_WHITE << "Running: " << Colors::GREEN
                << running << Colors::RESET
                << " | " << Colors::BRIGHT_WHITE << "Finished: " << Colors::YELLOW
                << finished << Colors::RESET
                << " | " << Colors::BRIGHT_WHITE << "CPU Ticks: " << Colors::BRIGHT_CYAN
                << ticks << Colors::RESET << std::flush;

            // Restore cursor position
            {
                std::lock_guard<std::mutex> lock(console_mutex);
                printf("\033[u");  // Restore cursor position
                fflush(stdout);
            }
        }
    }
}

// Display help
void display_help() {
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
        << "\n    - Stops automatic process generation and the scheduler\n";

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
    std::cout << "-----------------------------------------------------\n";

    // Show newest logs produced by the process instructions (PRINT etc.)
    auto logs = process->drain_logs();
    if (logs.empty()) {
        std::cout << Colors::WHITE << "  (no new output)" << Colors::RESET << "\n";
    } else {
        for (const auto& line : logs) {
            std::cout << Colors::GREEN << "  > " << Colors::WHITE << line << Colors::RESET << "\n";
        }
    }

    std::cout << "-----------------------------------------------------\n";

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

    auto processes = scheduler->get_all_processes();

    std::cout << Colors::BOLD << Colors::BRIGHT_CYAN << "\nProcess List:\n" << Colors::RESET;
    std::cout << "-------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(20) << "Name"
        << std::setw(12) << "State"
        << std::setw(8) << "Core"
        << std::setw(15) << "Progress"
        << "Created\n";
    std::cout << "-------------------------------------------------------------------------------------\n";

    for (auto& process : processes) {
        std::cout << std::left << std::setw(20) << process->get_name();

        // State with color
        std::string state = process->get_state_string();
        if (state == "Running") {
            std::cout << Colors::GREEN;
        }
        else if (state == "Finished") {
            std::cout << Colors::YELLOW;
        }
        else {
            std::cout << Colors::WHITE;
        }
        std::cout << std::setw(12) << state << Colors::RESET;

        // Core
        int core = process->get_core_id();
        std::cout << std::setw(8) << (core >= 0 ? std::to_string(core) : "N/A");

        // Progress
        int current = process->get_current_line();
        int total = process->get_total_commands();
        std::cout << std::setw(15) << (std::to_string(current) + "/" + std::to_string(total));

        // Timestamp
        std::cout << process->get_timestamp() << "\n";
    }

    std::cout << "-------------------------------------------------------------------------------------\n";
}

// Generate utilization report
void generate_report() {
    if (!scheduler) {
        std::cout << Colors::RED << "Scheduler not initialized!\n" << Colors::RESET;
        return;
    }

    int active, total, running, finished;
    scheduler->get_stats(active, total, running, finished);

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

    file << "CPU Cores: " << total << "\n";
    file << "Active Cores: " << active << "\n";
    file << "CPU Utilization: " << (total > 0 ? (active * 100.0 / total) : 0) << "%\n\n";
    file << "CPU Ticks: " << scheduler->get_cpu_ticks() << "\n\n";

    file << "Process Statistics:\n";
    file << "-------------------\n";
    file << "Running Processes: " << running << "\n";
    file << "Finished Processes: " << finished << "\n";
    file << "Total Processes: " << (running + finished) << "\n\n";

    file << "Process Details:\n";
    file << "----------------\n";

    auto processes = scheduler->get_all_processes();
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

    scheduler = std::make_unique<Scheduler>(NUM_CPU, SCHEDULER_TYPE, QUANTUM_CYCLES);
    scheduler->start();
    system_initialized = true;

    std::cout << Colors::BRIGHT_GREEN << "OS Emulator initialized successfully!\n" << Colors::RESET;
    std::cout << Colors::CYAN << "Scheduler type: " << SCHEDULER_TYPE << "\n";
    std::cout << "CPU cores: " << NUM_CPU << "\n" << Colors::RESET;
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
        int process_counter = 1;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 0);

        while (scheduler_autorun) {
            {
                std::lock_guard<std::mutex> lock(batch_mutex);
                std::string name = std::string("p") + (process_counter < 10 ? "0" : "") + std::to_string(process_counter);
                int instructions = 0;
                scheduler->add_process(name, instructions);

                std::cout << Colors::GREEN << "Generated process " << name
                    << " (" << scheduler->get_process(name)->get_total_commands() << " instructions)\n" << Colors::RESET;

                process_counter++;
            }

            // Sleep for configured frequency
            for (int i = 0; i < BATCH_PROCESS_FREQ * 10 && scheduler_autorun; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << Colors::BRIGHT_YELLOW << "Process generation stopped.\n" << Colors::RESET;
        });
}

// Handle 'scheduler-stop' command
void cmd_scheduler_stop() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized.\n" << Colors::RESET;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(batch_mutex);
        if (scheduler_autorun) {
            scheduler_autorun = false;
        }
    }

    if (batch_thread.joinable()) {
        batch_thread.join();
    }

    if (scheduler) {
        scheduler->stop();
    }

    std::cout << Colors::BRIGHT_YELLOW
        << "Process generation and scheduler stopped.\n"
        << Colors::RESET;
}

// Handle 'report-util' command
void cmd_report_util() {
    if (!system_initialized) {
        std::cout << Colors::RED << "Error: System not initialized. Run 'initialize' first.\n"
            << Colors::RESET;
        return;
    }

    generate_report();
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
    else if (cmd == "clear") {
        display_main_ui();
    }
    else if (cmd == "exit") {
        is_running = false;
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
    // Load configuration from file
    load_config();

    // Initialize terminal
    enable_ansi_on_windows();
    get_console_size(layout.screen_width, layout.screen_height);

    // Display initial UI
    display_main_ui();

    // Start CPU display update thread
    std::thread cpu_thread(cpu_display_thread);

    // Run keyboard handler in main thread
    keyboard_handler_thread();

    // Cleanup
    if (scheduler) {
        scheduler->stop();
    }

    scheduler_autorun = false;
    if (batch_thread.joinable()) {
        batch_thread.join();
    }

    if (cpu_thread.joinable()) {
        cpu_thread.join();
    }

    clear_screen();
    std::cout << Colors::BRIGHT_RED << "CSOPESY OS Emulator shutting down...\n" << Colors::RESET;
    std::cout << Colors::BRIGHT_YELLOW << "Thank you for using our system!\n" << Colors::RESET;

    return 0;
}
