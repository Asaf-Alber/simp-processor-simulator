#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 500  // Maximum line length for input lines
#define MAX_LABELS 1000      // Maximum number of labels supported
#define MEM_SIZE 4096        // Size of memory

// Memory overrides and flags for forced memory values
unsigned int mem_overrides[MEM_SIZE];
int mem_forced[MEM_SIZE];  // 0 = empty, 1 = force override with mem_overrides[address]

// Structure to hold label names and addresses
typedef struct {
    char name[50];
    int address;
} Label;

Label labels[MAX_LABELS];
int label_count = 0;

// Function declarations
int get_register_number(const char* reg);
int get_opcode_number(const char* opcode);
int get_label_address(const char* label);
void collect_labels(FILE* input);
void trim(char* str);

int main(int argc, char* argv[]) {
    //checks if we got 3 input files
    if (argc != 3) {
        printf("Usage: asm.exe program.asm memin.txt\n");
        return 1;
    }

    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "w");

    if (!input || !output) {
        printf("Error opening files.\n");
        return 1;
    }

    collect_labels(input);  // First pass to collect labels

    rewind(input);  // Rewind file pointer for second pass

    char line[MAX_LINE_LENGTH];
    int current_address = 0;  // Tracks current memory address

    while (fgets(line, sizeof(line), input)) {
        // Remove comments starting with '#'
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        // Skip empty or whitespace-only lines
        if (strlen(line) < 2) continue;

        // Check if line has a label and skip it
        char* colon = strchr(line, ':');
        if (colon) {
            memmove(line, colon + 1, strlen(colon + 1) + 1);
            trim(line);
            if (strlen(line) == 0) continue;  // Nothing left
        }

        // Ignore .word directives
        if (strncmp(line, ".word", 5) == 0)
            continue;

        // Parse the instruction line
        char opcode[20], rd[10], rs[10], rt[10], imm_str[50];
        int fields = sscanf(line, "%s %[^,], %[^,], %[^,], %s",
            opcode, rd, rs, rt, imm_str);

        if (fields < 5) {
            continue;  // Malformed instruction line
        }

        // Translate opcode and register names to numbers
        int op = get_opcode_number(opcode);
        int rd_num = get_register_number(rd);
        int rs_num = get_register_number(rs);
        int rt_num = get_register_number(rt);
        int imm_val = 0;
        int is_label = 0;

        trim(imm_str);

        // Parse immediate field
        if (strstr(imm_str, "0x") == imm_str || strstr(imm_str, "0X") == imm_str) {
            sscanf(imm_str, "%x", &imm_val);  // Hexadecimal
        }
        else if (isdigit(imm_str[0]) || imm_str[0] == '-') {
            sscanf(imm_str, "%d", &imm_val);  // Decimal
        }
        else {
            imm_val = get_label_address(imm_str);  // Label
            is_label = 1;
        }

        // Determine if a big immediate (more than 8 bits) is needed
        int bigimm = is_label || imm_val < -128 || imm_val > 127;

        // Encode instruction
        unsigned int inst = 0;
        inst |= (op & 0xFF) << 24;
        inst |= (rd_num & 0xF) << 20;
        inst |= (rs_num & 0xF) << 16;
        inst |= (rt_num & 0xF) << 12;
        inst |= (bigimm & 0x1) << 8;
        if (!bigimm) {
            inst |= (imm_val & 0xFF);  // 8-bit immediate
        }

        // Write instruction or memory override to output file
        if (mem_forced[current_address]) {
            fprintf(output, "%08X\n", mem_overrides[current_address]);
        }
        else {
            fprintf(output, "%08X\n", inst);
        }
        current_address++;

        // Write big immediate value if needed
        if (bigimm) {
            if (mem_forced[current_address]) {
                fprintf(output, "%08X\n", mem_overrides[current_address]);
            }
            else {
                fprintf(output, "%08X\n", imm_val);
            }
            current_address++;
        }
    }

    // Fill the rest of memory with zeros or forced overrides
    while (current_address < MEM_SIZE) {
        if (mem_forced[current_address]) {
            fprintf(output, "%08X\n", mem_overrides[current_address]);
        }
        else {
            fprintf(output, "00000000\n");
        }
        current_address++;
    }

    fclose(input);
    fclose(output);
    return 0;
}

// Map register name to register number
int get_register_number(const char* reg) {
    if (strcmp(reg, "$zero") == 0) return 0;
    if (strcmp(reg, "$imm") == 0) return 1;
    if (strcmp(reg, "$v0") == 0) return 2;
    if (strcmp(reg, "$a0") == 0) return 3;
    if (strcmp(reg, "$a1") == 0) return 4;
    if (strcmp(reg, "$a2") == 0) return 5;
    if (strcmp(reg, "$a3") == 0) return 6;
    if (strcmp(reg, "$t0") == 0) return 7;
    if (strcmp(reg, "$t1") == 0) return 8;
    if (strcmp(reg, "$t2") == 0) return 9;
    if (strcmp(reg, "$s0") == 0) return 10;
    if (strcmp(reg, "$s1") == 0) return 11;
    if (strcmp(reg, "$s2") == 0) return 12;
    if (strcmp(reg, "$gp") == 0) return 13;
    if (strcmp(reg, "$sp") == 0) return 14;
    if (strcmp(reg, "$ra") == 0) return 15;
    return -1;  // Invalid register
}

// Map opcode name to opcode number
int get_opcode_number(const char* opcode) {
    if (strcmp(opcode, "add") == 0) return 0;
    if (strcmp(opcode, "sub") == 0) return 1;
    if (strcmp(opcode, "mul") == 0) return 2;
    if (strcmp(opcode, "and") == 0) return 3;
    if (strcmp(opcode, "or") == 0) return 4;
    if (strcmp(opcode, "xor") == 0) return 5;
    if (strcmp(opcode, "sll") == 0) return 6;
    if (strcmp(opcode, "sra") == 0) return 7;
    if (strcmp(opcode, "srl") == 0) return 8;
    if (strcmp(opcode, "beq") == 0) return 9;
    if (strcmp(opcode, "bne") == 0) return 10;
    if (strcmp(opcode, "blt") == 0) return 11;
    if (strcmp(opcode, "bgt") == 0) return 12;
    if (strcmp(opcode, "ble") == 0) return 13;
    if (strcmp(opcode, "bge") == 0) return 14;
    if (strcmp(opcode, "jal") == 0) return 15;
    if (strcmp(opcode, "lw") == 0) return 16;
    if (strcmp(opcode, "sw") == 0) return 17;
    if (strcmp(opcode, "reti") == 0) return 18;
    if (strcmp(opcode, "in") == 0) return 19;
    if (strcmp(opcode, "out") == 0) return 20;
    if (strcmp(opcode, "halt") == 0) return 21;
    return -1;  // Invalid opcode
}

// Trim leading and trailing whitespace
void trim(char* str) {
    char* start = str;
    while (isspace((unsigned char)*start)) start++;

    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) *end-- = '\0';

    if (start != str) memmove(str, start, strlen(start) + 1);
}

// First pass: Collect labels and set memory overrides
void collect_labels(FILE* input) {
    char line[MAX_LINE_LENGTH];
    int address = 0;

    while (fgets(line, sizeof(line), input)) {
        // Remove comments
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        trim(line);

        if (strlen(line) == 0) continue;

        // Process .word directives
        if (strncmp(line, ".word", 5) == 0) {
            unsigned int addr = 0;
            long long temp_value = 0;
            if (sscanf(line + 5, "%x %x", &addr, &temp_value) == 2) {
                if (temp_value > 0xFFFFFFFF) {
                    printf("Error: .word value 0x%llX exceeds 32-bit limit.\n", temp_value);
                    exit(1);
                }
                if (addr >= MEM_SIZE) {
                    printf("Warning: .word address %u out of bounds.\n", addr);
                }
                else {
                    unsigned int value = (unsigned int)temp_value;
                    mem_overrides[addr] = value;
                    mem_forced[addr] = 1;
                }
            }
            else {
                printf("Malformed .word directive: %s\n", line);
            }
            continue;
        }

        // Check if the line defines a label
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            trim(line);
            strcpy(labels[label_count].name, line);
            labels[label_count].address = address;
            label_count++;
            continue;
        }

        // Parse the instruction to see if big immediate is needed
        char opcode[20], rd[10], rs[10], rt[10], imm_str[50];
        int fields = sscanf(line, "%s %[^,], %[^,], %[^,], %s",
            opcode, rd, rs, rt, imm_str);

        if (fields < 5) {
            continue;  // Malformed instruction
        }

        trim(imm_str);

        int imm_val = 0;
        int bigimm = 0;

        if (strstr(imm_str, "0x") == imm_str || strstr(imm_str, "0X") == imm_str) {
            sscanf(imm_str, "%x", &imm_val);  // Hexadecimal
        }
        else if (isdigit(imm_str[0]) || imm_str[0] == '-') {
            sscanf(imm_str, "%d", &imm_val);  // Decimal
        }
        else {
            bigimm = 1;  // Assume label requires big immediate
        }

        if (!bigimm && (imm_val < -128 || imm_val > 127)) {
            bigimm = 1;
        }

        address++;  // Increment for normal instruction
        if (bigimm) {
            address++;  // Extra word for big immediate
        }
    }
}

// Lookup address by label name
int get_label_address(const char* label) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, label) == 0) {
            return labels[i].address;
        }
    }
    printf("Error: label not found: %s\n", label);
    exit(1);
}
