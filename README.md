# SIMP Processor Toolchain  
### Assembler & Cycle-Accurate Simulator (ANSI C)

## Overview

This project implements a complete software toolchain for the SIMP processor, a minimalist 32-bit RISC-style architecture inspired by MIPS.

The system includes:

- An assembler that translates SIMP assembly into executable machine code  
- A cycle-accurate simulator that emulates CPU execution, memory, I/O, interrupts, disk operations, and graphical output  

The goal was to build a fully functional execution environment that models how a real processor interacts with memory and peripherals at the instruction level.

---

```mermaid
flowchart TD

  %% ===== ASSEMBLER =====
  A[Write SIMP Assembly Program (.asm)]
      --> B[Assembler (Two-Pass, C)]
  B --> C[memin.txt<br/>4096 x 32-bit words]

  %% ===== SIMULATOR INPUTS =====
  C --> D[Cycle-Accurate Simulator (C)]
  E[diskin.txt<br/>Initial Disk Image] --> D
  F[irq2in.txt<br/>External IRQ Schedule] --> D

  %% ===== EXECUTION LOOP =====
  D --> G{Per-Cycle Execution Loop}

  G --> H[1. Interrupt Check<br/>(irq0 / irq1 / irq2)]
  H --> I[2. Fetch<br/>Instruction = Memory[PC]]
  I --> J[3. Decode<br/>Opcode / Registers / Immediates]
  J --> K[4. Execute<br/>ALU / Load-Store / Branch / I-O]
  K --> L[5. Update PC & Cycle Counter]
  L --> M[6. Update Peripherals<br/>Disk / LEDs / Display / Framebuffer]
  M --> N[7. Trace Logging<br/>trace.txt / hwregtrace.txt]

  N --> O{halt or MAX_CYCLES?}
  O -- No --> G
  O -- Yes --> P[Generate Final Outputs]

  %% ===== OUTPUTS =====
  P --> Q[memout.txt]
  P --> R[regout.txt]
  P --> S[cycles.txt]
  P --> T[diskout.txt]
  P --> U[monitor.txt / monitor.yuv]
  P --> V[leds.txt / display7seg.txt]
```

## Architecture

- 32-bit word size  
- 4096-word unified memory  
- 16 general-purpose registers  
- 23 memory-mapped I/O registers  
- Interrupt system (irq0, irq1, irq2)  
- Disk emulation (128 × 128-word sectors)  
- 256×256 framebuffer display  

The simulator executes programs cycle-by-cycle using a classic fetch-decode-execute loop, including interrupt handling and peripheral updates.

---

## Assembler

The assembler converts SIMP assembly programs into a 4096-word memory image (`memin.txt`).

Key aspects:

- Full ISA support (arithmetic, logic, memory, branching, I/O)
- Two-pass label resolution
- Immediate handling (8-bit and extended 32-bit values)
- Memory initialization via `.word` directives
- Strict binary encoding according to the SIMP instruction format

This component required implementing symbol tables, instruction encoding logic, and careful handling of control flow and immediates.

---

## Simulator

The simulator emulates:

- CPU register file and program counter  
- Memory subsystem  
- Disk controller  
- Interrupt mechanism  
- Memory-mapped I/O devices  
- Framebuffer graphics output  

Each clock cycle performs:

1. Interrupt check  
2. Instruction fetch  
3. Decode and operand extraction  
4. Execution  
5. PC update  
6. Peripheral and trace updates  

Execution continues until `halt` or a cycle limit is reached.

---

## Validation Programs

To verify correctness, multiple assembly programs were written:

- Bubble Sort  
- Recursive Factorial  
- Framebuffer rectangle renderer  
- Disk sector summation with interrupt coordination  

These programs stress-tested arithmetic, branching, memory access, recursion depth, disk I/O, and interrupt handling.

---

## What This Project Demonstrates

- Understanding of instruction set architecture (ISA) design  
- Binary encoding and assembly translation  
- Cycle-accurate processor simulation  
- Memory-mapped I/O modeling  
- Interrupt handling mechanisms  
- Hardware-software interaction at the system level  
- Low-level debugging using execution traces  

This project strengthened my understanding of how a processor executes instructions, manages control flow, interfaces with peripherals, and maintains architectural state across clock cycles.
