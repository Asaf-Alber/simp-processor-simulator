# SIMP Processor Toolchain  
### Assembler & Cycle-Accurate Simulator (ANSI C)

---

## 📌 Overview

This project implements a complete development and execution environment for the **SIMP processor**, a minimalist RISC-style architecture conceptually inspired by MIPS.

The toolchain consists of:

- 🛠 **Assembler** – Translates SIMP assembly programs into executable machine code  
- 🖥 **Cycle-Accurate Simulator** – Emulates the SIMP CPU, memory system, I/O devices, disk subsystem, interrupts, and graphical output  

The system enables full software-based execution and validation of SIMP programs without physical hardware.

---

## 🏗 Architecture Summary

- 32-bit word size  
- 4096-word unified memory  
- 16 general-purpose registers  
- 23 memory-mapped I/O registers  
- Interrupt support (`irq0`, `irq1`, `irq2`)  
- Disk emulation (128 sectors × 128 words)  
- 256×256 framebuffer display  
- LED and 7-segment peripheral simulation  

---

# 🛠 Assembler

## Purpose

Converts SIMP assembly code into a binary memory image (`memin.txt`) ready for simulator execution.

## Key Features

- ✔ Full ISA coverage  
  - Arithmetic  
  - Bitwise logic  
  - Load / Store  
  - Branching & Jumps  
  - I/O instructions  

- ✔ Immediate handling  
  - 8-bit immediates  
  - 32-bit extended immediates (automatic expansion)

- ✔ Two-pass assembly process  
  - First pass: label detection + symbol table creation  
  - Second pass: instruction encoding + label resolution  

- ✔ `.word` directive support (hex format required)  
- ✔ Comment stripping & preprocessing  
- ✔ Zero-padding unused memory  

## Output

Produces:
