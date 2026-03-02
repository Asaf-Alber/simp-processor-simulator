@"
# SIMP Processor Toolchain  
### Assembler & Cycle-Accurate Simulator (ANSI C)

---

## 📌 Overview

This project implements a complete development and execution environment for the SIMP processor, a minimalist RISC-style architecture conceptually inspired by MIPS.

The toolchain consists of:

- 🛠 Assembler – Translates SIMP assembly programs into executable machine code  
- 🖥 Cycle-Accurate Simulator – Emulates the SIMP CPU, memory system, I/O devices, disk subsystem, interrupts, and graphical output  

The system enables full software-based execution and validation of SIMP programs without physical hardware.

---

## 🏗 Architecture Summary

- 32-bit word size  
- 4096-word unified memory  
- 16 general-purpose registers  
- 23 memory-mapped I/O registers  
- Interrupt support (irq0, irq1, irq2)  
- Disk emulation (128 sectors × 128 words)  
- 256×256 framebuffer display  
- LED and 7-segment peripheral simulation  

---

# 🛠 Assembler

## Purpose

Converts SIMP assembly code into a binary memory image (memin.txt) ready for simulator execution.

## Key Features

- Full ISA coverage  
  - Arithmetic  
  - Bitwise logic  
  - Load / Store  
  - Branching & Jumps  
  - I/O instructions  

- Immediate handling  
  - 8-bit immediates  
  - 32-bit extended immediates (automatic expansion)

- Two-pass assembly process  
  - First pass: label detection + symbol table creation  
  - Second pass: instruction encoding + label resolution  

- .word directive support (hex format required)  
- Comment stripping & preprocessing  
- Zero-padding unused memory  

## Output

Produces:

    memin.txt

Exactly 4096 lines of 32-bit hexadecimal words representing initialized memory.

---

# 🖥 Simulator

## Purpose

Implements a full software execution environment for SIMP machine code using a classic:

    Fetch → Decode → Execute

loop executed cycle-by-cycle until:
- halt instruction  
- or maximum cycle threshold  

---

## 🧠 Internal Components

### CPU Core
- Program Counter (PC)
- Clock cycle counter
- 16 General Purpose Registers

### Memory
- 4096 × 32-bit words
- Unified instruction + data memory

### Disk Subsystem
- 128 sectors
- 128 words per sector
- Full read/write emulation

### Display System
- 256×256 framebuffer
- Memory-mapped graphics output
- Outputs:
  - monitor.txt
  - monitor.yuv

### Interrupt System

Supports three interrupt lines:
- irq0 – Timer  
- irq1 – Disk  
- irq2 – External  

Interrupt handling flow:
1. Detect active interrupt  
2. Save CPU state  
3. Jump to irqhandler  
4. Return via reti using irqreturn  

---

## 🔁 Execution Cycle

Each clock cycle performs:
1. Interrupt check  
2. Instruction fetch  
3. Decode (opcode, registers, immediates)  
4. Sign extension  
5. Execute  
6. Update PC and clock counter  
7. Trace logging  
8. Peripheral updates  

---

## 📂 Input Files

- memin.txt – Assembled memory image  
- diskin.txt – Initial disk contents  
- irq2in.txt – External interrupt schedule  

---

## 📤 Output Files

After execution:
- memout.txt – Final memory state  
- regout.txt – Register values  
- cycles.txt – Total cycles executed  
- diskout.txt – Final disk state  
- monitor.txt – Framebuffer (hex)  
- monitor.yuv – Framebuffer image  
- leds.txt  
- display7seg.txt  
- trace.txt  
- hwregtrace.txt  

---

# 🧪 Validation Programs

1. Bubble Sort – Tests arithmetic, memory operations, and branching  
2. Recursive Factorial – Validates control flow depth  
3. Graphics Rectangle Renderer – Framebuffer manipulation  
4. Disk Sector Summation – Disk reads, summation, interrupt coordination  

---

# 🏗 Build & Environment

- Language: ANSI C (C89)  
- Development Environment: Microsoft Visual Studio 2022  
- Console-based applications  

### Execution Flow

1. Run Assembler → Generates memin.txt  
2. Run Simulator → Executes program → Produces logs  

---

# 🎓 Engineering Value

This project demonstrates:

- Instruction Set Architecture implementation  
- Binary encoding and symbol resolution  
- Cycle-accurate CPU simulation  
- Memory-mapped I/O design  
- Interrupt handling mechanisms  
- Disk and framebuffer emulation  
- Trace-based debugging  
- Strong foundation in computer architecture  

---
