
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// --- General Hardware Definitions ---
#define MEMORY_WORD_COUNT 4096       // Total words in main memory (4K)
#define REGISTER_COUNT 16            // Number of general-purpose registers
#define IO_REGISTER_COUNT 23         // Number of I/O registers
#define DISK_SECTOR_COUNT 128        // Number of sectors on the disk
#define DISK_SECTOR_SIZE 128         // Size of each disk sector in words
#define MONITOR_WIDTH 256            // Width of the monitor in pixels
#define MONITOR_HEIGHT 256           // Height of the monitor in pixels
#define DISK_OPERATION_CYCLES 1024   // Number of cycles a disk operation takes
#define SIMULATION_MAX_CYCLES 500000 // Maximum cycles to run to prevent infinite loops

// --- Debugging Configuration ---
// Set DEBUG_MODE to 1 to enable verbose printing to stderr during execution.
#define DEBUG_MODE 0
#define DEBUG_PRINT(...) do { if (DEBUG_MODE) fprintf(stderr, __VA_ARGS__); } while (0)

// --- CPU State and Memory Structures ---
uint32_t memory[MEMORY_WORD_COUNT] = { 0 };
uint32_t registers[REGISTER_COUNT] = { 0 };
uint32_t io_registers[IO_REGISTER_COUNT] = { 0 };
uint32_t monitor[MONITOR_HEIGHT][MONITOR_WIDTH] = { 0 };
uint32_t disk[DISK_SECTOR_COUNT][DISK_SECTOR_SIZE] = { 0 };

uint32_t program_counter = 0;
uint32_t cycle_count = 0;

// State tracking for output generation and debugging
uint32_t previous_registers_state[REGISTER_COUNT] = { 0 };
uint32_t previous_io_registers_state[IO_REGISTER_COUNT] = { 0 };
uint32_t max_memory_address_written = 0;
int max_monitor_pixel_written = -1;

// Interrupt related state
bool is_in_interrupt_handler = false;
int irq2_trigger_cycles[SIMULATION_MAX_CYCLES];
int irq2_schedule_count = 0;

// --- File Pointers for Input/Output ---
FILE* memout_fp;
FILE* regout_fp;
FILE* trace_fp;
FILE* hwregtrace_fp;
FILE* cycles_fp;
FILE* leds_fp;
FILE* display7seg_fp;
FILE* diskout_fp;
FILE* monitor_fp;
FILE* monitor_yuv_fp;


// --- Forward Declarations ---
void load_input_files(const char* memin_path, const char* diskin_path, const char* irq2in_path);
void write_output_files(void);
void run_simulation_loop(void);
void handle_interrupts(void);
void execute_instruction(uint32_t instruction);
const char* get_io_register_name(int index);
void trace_decoded_instruction(uint32_t instruction);
void trace_register_changes(void);
void trace_io_register_changes(void);


// Helper function to sign-extend an 8-bit immediate value to 32 bits.
int32_t sign_extend(uint8_t imm8) {
    // If the 7th bit (sign bit) is 1, it's a negative number.
    if (imm8 & 0x80) {
        return (int32_t)(0xFFFFFF00 | imm8);
    }
    else {
        return (int32_t)imm8;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 14) {
        fprintf(stderr, "Usage: sim.exe memin.txt diskin.txt irq2in.txt memout.txt regout.txt "
            "trace.txt hwregtrace.txt cycles.txt leds.txt display7seg.txt "
            "diskout.txt monitor.txt monitor.yuv\n");
        return 1;
    }

    // Assign command-line arguments to meaningful variables
    const char* memin_path = argv[1];
    const char* diskin_path = argv[2];
    const char* irq2in_path = argv[3];
    const char* memout_path = argv[4];
    const char* regout_path = argv[5];
    const char* trace_path = argv[6];
    const char* hwregtrace_path = argv[7];
    const char* cycles_path = argv[8];
    const char* leds_path = argv[9];
    const char* display7seg_path = argv[10];
    const char* diskout_path = argv[11];
    const char* monitor_path = argv[12];
    const char* monitor_yuv_path = argv[13];

    // Initialize all memory and register structures to zero.
    memset(io_registers, 0, sizeof(io_registers));
    memset(registers, 0, sizeof(registers));
    memset(previous_registers_state, 0, sizeof(previous_registers_state));
    memset(previous_io_registers_state, 0, sizeof(previous_io_registers_state));

    // Load initial state from input files.
    load_input_files(memin_path, diskin_path, irq2in_path);

    // Open output files for writing.
    trace_fp = fopen(trace_path, "w");
    hwregtrace_fp = fopen(hwregtrace_path, "w");
    leds_fp = fopen(leds_path, "w");
    display7seg_fp = fopen(display7seg_path, "w");

    if (!trace_fp || !hwregtrace_fp || !leds_fp || !display7seg_fp) {
        perror("Failed to open one or more initial output files");
        return 1;
    }

    // Start the main simulation loop.
    run_simulation_loop();

    // Open the rest of the output files to write final state.
    memout_fp = fopen(memout_path, "w");
    regout_fp = fopen(regout_path, "w");
    cycles_fp = fopen(cycles_path, "w");
    diskout_fp = fopen(diskout_path, "w");
    monitor_fp = fopen(monitor_path, "w");
    monitor_yuv_fp = fopen(monitor_yuv_path, "wb");

    if (!memout_fp || !regout_fp || !cycles_fp || !diskout_fp || !monitor_fp || !monitor_yuv_fp) {
        perror("Failed to open one or more final output files");
        // Close already opened files before exiting
        fclose(trace_fp);
        fclose(hwregtrace_fp);
        fclose(leds_fp);
        fclose(display7seg_fp);
        return 1;
    }

    // Write the final state of the simulation to the output files.
    write_output_files();

    return 0;
}


// Loads the initial memory, disk, and interrupt schedule from input files.
void load_input_files(const char* memin_path, const char* diskin_path, const char* irq2in_path) {
    FILE* fp;
    char line_buffer[100];
    int address_counter = 0;

    // --- Load memin.txt into main memory ---
    fp = fopen(memin_path, "r");
    if (!fp) {
        perror("Failed to open memin.txt");
        exit(1);
    }
    while (fgets(line_buffer, sizeof(line_buffer), fp) && address_counter < MEMORY_WORD_COUNT) {
        sscanf(line_buffer, "%x", &memory[address_counter++]);
    }
    max_memory_address_written = address_counter;
    fclose(fp);

    // --- Load diskin.txt into the disk simulation ---
    fp = fopen(diskin_path, "r");
    if (!fp) {
        perror("Failed to open diskin.txt");
        exit(1);
    }
    for (int sector = 0; sector < DISK_SECTOR_COUNT; sector++) {
        for (int word = 0; word < DISK_SECTOR_SIZE; word++) {
            if (!fgets(line_buffer, sizeof(line_buffer), fp)) break;
            sscanf(line_buffer, "%x", &disk[sector][word]);
        }
    }
    fclose(fp);

    // --- Load irq2in.txt for scheduled hardware interrupts ---
    fp = fopen(irq2in_path, "r");
    if (!fp) {
        perror("Failed to open irq2in.txt");
        exit(1);
    }
    irq2_schedule_count = 0;
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        sscanf(line_buffer, "%d", &irq2_trigger_cycles[irq2_schedule_count++]);
    }
    fclose(fp);
}


// Writes the final state of memory, registers, and devices to output files.
void write_output_files(void) {
    // Write memory contents to memout.txt
    for (uint32_t i = 0; i < max_memory_address_written; i++) {
        fprintf(memout_fp, "%08X\n", memory[i]);
    }
    fclose(memout_fp);

    // Write register contents (R2-R15) to regout.txt
    for (int i = 2; i < REGISTER_COUNT; i++) {
        fprintf(regout_fp, "%08X\n", registers[i]);
    }
    fclose(regout_fp);

    // Write total cycle count to cycles.txt
    fprintf(cycles_fp, "%08X\n", cycle_count);
    fclose(cycles_fp);

    // Close trace files that were written to during the simulation
    fclose(trace_fp);
    fclose(hwregtrace_fp);
    fclose(leds_fp);
    fclose(display7seg_fp);

    // Write disk contents to diskout.txt
    for (int s = 0; s < DISK_SECTOR_COUNT; s++) {
        for (int w = 0; w < DISK_SECTOR_SIZE; w++) {
            fprintf(diskout_fp, "%08X\n", disk[s][w]);
        }
    }
    fclose(diskout_fp);

    // Write monitor pixel data to monitor.txt (hex)
    for (int i = 0; i <= max_monitor_pixel_written; i++) {
        int y = i / MONITOR_WIDTH;
        int x = i % MONITOR_WIDTH;
        fprintf(monitor_fp, "%02X\n", monitor[y][x] & 0xFF);
    }
    fclose(monitor_fp);

    // Write monitor pixel data to monitor.yuv (raw binary)
    for (int y = 0; y < MONITOR_HEIGHT; y++) {
        for (int x = 0; x < MONITOR_WIDTH; x++) {
            uint8_t pixel_value = monitor[y][x] & 0xFF;
            fwrite(&pixel_value, 1, 1, monitor_yuv_fp);
        }
    }
    fclose(monitor_yuv_fp);
}


// Checks for and handles pending interrupts at the start of a new cycle.
void handle_interrupts(void) {
    // Update the clock I/O register with the current cycle count.
    io_registers[8] = cycle_count;

    // --- Timer Interrupt (IRQ0) ---
    // Check if the timer is enabled.
    if (io_registers[11]) {
        io_registers[12]++; // Increment timercurrent
        // If timercurrent reaches its maximum value, trigger IRQ0.
        if (io_registers[12] >= io_registers[13]) {
            io_registers[12] = 0;
            io_registers[3] = 1; // Set irq0status to 1
        }
    }

    // --- External Interrupt (IRQ2) ---
    // Check if the current cycle count matches any scheduled IRQ2 time.
    for (int i = 0; i < irq2_schedule_count; i++) {
        if (irq2_trigger_cycles[i] == cycle_count) {
            io_registers[5] = 1; // Set irq2status to 1
            break;
        }
    }

    // Determine if any enabled interrupt has its status flag set.
    bool irq0_active = io_registers[0] & io_registers[3];
    bool irq1_active = io_registers[1] & io_registers[4];
    bool irq2_active = io_registers[2] & io_registers[5];
    bool any_irq_pending = irq0_active || irq1_active || irq2_active;

    DEBUG_PRINT("[IRQ CHECK] cycle=%u | status=%u%u%u | enable=%u%u%u → pending=%d\n",
        cycle_count,
        io_registers[3], io_registers[4], io_registers[5],
        io_registers[0], io_registers[1], io_registers[2],
        any_irq_pending);

    // If an interrupt is pending and we are not already in the handler
    if (any_irq_pending && !is_in_interrupt_handler) {
        DEBUG_PRINT("[INTERRUPT] Triggered at cycle=%u\n", cycle_count);

        is_in_interrupt_handler = true;
        // The hardware automatically clears the status bit of the interrupt being handled.
        // The specification is ambiguous about priority, we assume any active interrupt is handled.
        // For simplicity here, we can just clear them all upon entry. A real CPU would have a priority encoder.
        // Based on the 'reti' instruction clearing all of them, this seems plausible.

        uint32_t irq_return_address = program_counter;
        uint32_t irq_handler_address = io_registers[6];

        // Save the return address and jump to the interrupt handler.
        io_registers[7] = irq_return_address;
        program_counter = irq_handler_address;
    }
}



// The main loop of the simulator: fetches, decodes, and executes instructions.
void run_simulation_loop(void) {
    while (true) {
        if (cycle_count > SIMULATION_MAX_CYCLES) {
            fprintf(stderr, "Exceeded %u cycles — simulation halted.\n", SIMULATION_MAX_CYCLES);
            break;
        }

        // --- Interrupt Handling ---
        // Check for interrupts at the beginning of each cycle.
        handle_interrupts();

        // --- Fetch ---
        uint32_t current_pc = program_counter;
        if (current_pc >= MEMORY_WORD_COUNT) {
            fprintf(stderr, "ERROR: Program counter out of bounds at PC=%03X\n", current_pc);
            break;
        }
        uint32_t instruction = memory[current_pc];

        // --- Decode ---
        uint8_t opcode = instruction >> 24;
        trace_decoded_instruction(instruction);

        // --- Tracing ---
        // Write the state BEFORE execution to the trace file.
        fprintf(trace_fp, "%08X %03X %08X", cycle_count, current_pc, instruction);
        for (int i = 0; i < REGISTER_COUNT; i++) {
            // R0 is always 0, R1 represents the immediate for tracing purposes.
            uint32_t trace_val = registers[i];
            if (i == 0) trace_val = 0;
            fprintf(trace_fp, " %08X", trace_val);
        }
        fprintf(trace_fp, "\n");

        // --- Execute ---
        execute_instruction(instruction);

        // --- Cycle and State Updates ---
        cycle_count++;

        // Log changes to registers and I/O registers for debugging.
        trace_register_changes();
        trace_io_register_changes();

        // --- Disk Operation Completion Check (IRQ1) ---
        // The diskstatus register (IO[17]) indicates if an operation is busy.
        if (io_registers[17] == 1 && cycle_count >= io_registers[18]) {
            int command = io_registers[14];
            int sector = io_registers[15];
            int buffer_address = io_registers[16];

            if (sector >= DISK_SECTOR_COUNT) {
                fprintf(stderr, "ERROR: Disk sector out of range (%d)\n", sector);
                exit(1);
            }

            // Command 1: Read from disk to memory
            if (command == 1) {
                for (int i = 0; i < DISK_SECTOR_SIZE; i++) {
                    memory[(buffer_address + i) & (MEMORY_WORD_COUNT - 1)] = disk[sector][i];
                }
            }
            // Command 2: Write from memory to disk
            else if (command == 2) {
                for (int i = 0; i < DISK_SECTOR_SIZE; i++) {
                    disk[sector][i] = memory[(buffer_address + i) & (MEMORY_WORD_COUNT - 1)];
                }
            }

            // Mark disk as no longer busy and trigger IRQ1.
            io_registers[17] = 0; // diskstatus = 0
            io_registers[14] = 0; // diskcmd = 0
            io_registers[4] = 1;  // irq1status = 1
        }

        // Opcode 21 is HALT.
        if (opcode == 21) {
            break;
        }
    }
}


/* Executes a single instruction.
   Decodes the instruction and performs the corresponding operation on the
   registers, memory, or I/O devices. Updates the program counter accordingly.
*/
void execute_instruction(uint32_t instruction) {
    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t rd = (instruction >> 20) & 0xF;
    uint8_t rs = (instruction >> 16) & 0xF;
    uint8_t rt = (instruction >> 12) & 0xF;
    uint8_t has_extended_immediate = (instruction >> 8) & 0x1;

    int32_t immediate_value;

    // Determine if the instruction uses an 8-bit immediate or a 32-bit extended immediate.
    // The extended immediate is stored in the memory word following the instruction.
    if (has_extended_immediate) {
        if (program_counter + 1 >= MEMORY_WORD_COUNT) {
            fprintf(stderr, "ERROR: bigimm access out of memory at PC=%03X\n", program_counter);
            exit(1);
        }
        immediate_value = memory[program_counter + 1];
    }
    else {
        immediate_value = sign_extend(instruction & 0xFF);
    }

    // An instruction word and its potential extended immediate count for program counter advancement.
    int instruction_size_in_words = has_extended_immediate ? 2 : 1;

    // Get source register values. If a source register is $1, use the immediate value instead.
    // Register $0 is hardwired to zero.
    uint32_t val_rs = (rs == 0) ? 0 : ((rs == 1) ? immediate_value : registers[rs]);
    uint32_t val_rt = (rt == 0) ? 0 : ((rt == 1) ? immediate_value : registers[rt]);

    // Default behavior is to advance PC by instruction size. Jumps will override this.
    program_counter += instruction_size_in_words;

    switch (opcode) {
    case 0:  registers[rd] = val_rs + val_rt; break; // add
    case 1:  registers[rd] = val_rs - val_rt; break; // sub
    case 2:  registers[rd] = val_rs * val_rt; break; // mul
    case 3:  registers[rd] = val_rs & val_rt; break; // and
    case 4:  registers[rd] = val_rs | val_rt; break; // or
    case 5:  registers[rd] = val_rs ^ val_rt; break; // xor
    case 6:  registers[rd] = val_rs << val_rt; break; // sll
    case 7:  registers[rd] = ((int32_t)val_rs) >> val_rt; break; // sra (arithmetic)
    case 8:  registers[rd] = val_rs >> val_rt; break; // srl (logical)
    case 9:  if (val_rs == val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // beq
    case 10: if (val_rs != val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // bne
    case 11: if ((int32_t)val_rs < (int32_t)val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // blt
    case 12: if ((int32_t)val_rs > (int32_t)val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // bgt
    case 13: if ((int32_t)val_rs <= (int32_t)val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // ble
    case 14: if ((int32_t)val_rs >= (int32_t)val_rt) program_counter = (has_extended_immediate) ? immediate_value : registers[rd]; break; // bge
    case 15: // jal
        registers[rd] = program_counter; // Return address is PC after this instruction
        program_counter = (has_extended_immediate) ? immediate_value : val_rs;
        break;
    case 16: // lw (load word)
        registers[rd] = memory[(val_rs + val_rt) & (MEMORY_WORD_COUNT - 1)];
        break;
    case 17: { // sw (store word)
        uint32_t address = (val_rs + val_rt) & (MEMORY_WORD_COUNT - 1);
        memory[address] = registers[rd];
        if (address >= max_memory_address_written) {
            max_memory_address_written = address + 1;
        }
        break;
    }
    case 18: // reti
        program_counter = io_registers[7];
        is_in_interrupt_handler = false;
        break;
    case 19: { // in (read from I/O register)
        int io_index = (val_rs + val_rt) % IO_REGISTER_COUNT;
        registers[rd] = io_registers[io_index];
        fprintf(hwregtrace_fp, "%08X READ %s %08X\n", cycle_count, get_io_register_name(io_index), registers[rd]);
        break;
    }
    case 20: { // out (write to I/O register)
        int io_index = (val_rs + val_rt) % IO_REGISTER_COUNT;
        uint32_t value_to_write = registers[rd];
        io_registers[io_index] = value_to_write;
        fprintf(hwregtrace_fp, "%08X WRITE %s %08X\n", cycle_count, get_io_register_name(io_index), value_to_write);

        // Handle side effects of writing to specific I/O registers
        if (io_index == 9) { // leds
            fprintf(leds_fp, "%08X %08X\n", cycle_count, value_to_write);
        }
        else if (io_index == 10) { // display7seg
            fprintf(display7seg_fp, "%08X %08X\n", cycle_count, value_to_write);
        }
        else if (io_index == 14 && value_to_write != 0) { // diskcmd
            io_registers[17] = 1; // Set diskstatus to busy
            io_registers[18] = cycle_count + DISK_OPERATION_CYCLES; // Set finish time
        }
        else if (io_index == 22 && value_to_write == 1) { // monitorcmd
            uint32_t monitor_address = io_registers[20];
            uint32_t y = monitor_address / MONITOR_WIDTH;
            uint32_t x = monitor_address % MONITOR_WIDTH;
            if (y < MONITOR_HEIGHT && x < MONITOR_WIDTH) {
                monitor[y][x] = io_registers[21] & 0xFF; // Set pixel from monitordata
                int linear_address = y * MONITOR_WIDTH + x;
                if (linear_address > max_monitor_pixel_written) {
                    max_monitor_pixel_written = linear_address;
                }
            }
            io_registers[22] = 0; // Command is consumed
        }
        break;
    }
    case 21: // halt
        // The main loop will detect this opcode and terminate.
        program_counter -= instruction_size_in_words; // Stay on HALT instruction
        break;
    default:
        fprintf(stderr, "ERROR: Unknown opcode %d at PC=%03X\n", opcode, program_counter - instruction_size_in_words);
        break;
    }

    // Register $0 must always be zero.
    registers[0] = 0;
    // Register $1 is not a real register; it represents the immediate for the current instruction.
    registers[1] = immediate_value;
}


// --- Debugging and Tracing Helper Functions ---


// Returns the string name of an I/O register given its index.
const char* get_io_register_name(int index) {
    static const char* IO_REGISTER_NAMES[IO_REGISTER_COUNT] = {
        "irq0enable", "irq1enable", "irq2enable",
        "irq0status", "irq1status", "irq2status",
        "irqhandler", "irqreturn", "clks",
        "leds", "display7seg", "timerenable",
        "timercurrent", "timermax", "diskcmd",
        "disksector", "diskbuffer", "diskstatus",
        "reserved18", "reserved19", "monitoraddr",
        "monitordata", "monitorcmd"
    };
    if (index >= 0 && index < IO_REGISTER_COUNT) {
        return IO_REGISTER_NAMES[index];
    }
    return "INVALID_IO_REG";
}


// Prints decoded instruction details if DEBUG_MODE is enabled.
void trace_decoded_instruction(uint32_t instruction) {
    if (!DEBUG_MODE) return;
    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t rd = (instruction >> 20) & 0xF;
    uint8_t rs = (instruction >> 16) & 0xF;
    uint8_t rt = (instruction >> 12) & 0xF;
    uint8_t has_extended_immediate = (instruction >> 8) & 0x1;
    uint8_t imm8 = instruction & 0xFF;

    DEBUG_PRINT("Decode: PC=%03X | OPCODE=%02d RD=%d RS=%d RT=%d BIGIMM=%d IMM8=0x%02X\n",
        program_counter, opcode, rd, rs, rt, has_extended_immediate, imm8);
}


// Prints register value changes if DEBUG_MODE is enabled.
void trace_register_changes(void) {
    if (!DEBUG_MODE) return;
    for (int i = 0; i < REGISTER_COUNT; i++) {
        if (registers[i] != previous_registers_state[i]) {
            DEBUG_PRINT("REG CHANGE R%-2d: %08X -> %08X\n", i, previous_registers_state[i], registers[i]);
            previous_registers_state[i] = registers[i];
        }
    }
}


// Prints I/O register value changes if DEBUG_MODE is enabled.
void trace_io_register_changes(void) {
    if (!DEBUG_MODE) return;
    for (int i = 0; i < IO_REGISTER_COUNT; i++) {
        if (io_registers[i] != previous_io_registers_state[i]) {
            DEBUG_PRINT("IOREG CHANGE %-13s: %08X -> %08X\n", get_io_register_name(i), previous_io_registers_state[i], io_registers[i]);
            previous_io_registers_state[i] = io_registers[i];
        }
    }
}
