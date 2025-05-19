# CSEN601 - Processor Simulation Project (Package 4)

This project is a C-based simulation of a 16-bit pipelined processor based on the "Package 4: Double McHarvard with cheese circular shifts" specification for the CSEN601 Computer Systems Architecture course.

## Processor Architecture Highlights:

*   **Architecture Type:** Harvard Architecture (separate instruction and data memories).
*   **Instruction Memory:** 1024 words x 16 bits.
*   **Data Memory:** 2048 words x 8 bits.
*   **Registers:**
    *   64 General-Purpose Registers (GPRs), 8-bit each (R0-R63).
    *   1 Status Register (SREG), 8-bit, with C, V, N, S, Z flags.
    *   1 Program Counter (PC), 16-bit.
*   **Instruction Set:** 12 instructions, 16-bit fixed length, including arithmetic, logical, load/store, branch, jump, and circular shift operations.
*   **Pipeline:** 3 stages:
    1.  Instruction Fetch (IF)
    2.  Instruction Decode (ID) & Operand Read
    3.  Execute (EX), Memory Access & Write-Back
*   **Hazard Handling:**
    *   **Control Hazards:** Handled by flushing the pipeline (IF and ID stages) on taken branches (`BEQZ`) and jumps (`JR`). The target instruction is fetched in the subsequent cycle.
    *   **Data Hazards (RAW):** Handled by stalling the pipeline for 1 cycle. If an instruction in EX writes to a register that an instruction in ID reads, a NOP bubble is inserted into EX, and the ID and IF stages are stalled.

## Features Implemented:

*   Parsing of assembly language programs from a text file.
*   Cycle-accurate simulation of the 3-stage pipeline.
*   Detailed pipeline state printing at each cycle, including register and memory updates.
*   Implementation of all 12 specified instructions for Package 4.
*   Correct SREG flag updates based on instruction execution.
*   Control hazard resolution via pipeline flushing.
*   Data hazard (Read-After-Write) resolution via pipeline stalling.
*   Calculation and display of total execution cycles and stall cycles.

## How to Compile and Run:

1.  **Compile:**
    ```bash
    gcc main.c -o processor_sim -std=c99
    ```
    *(Note: `-std=c99` or later is recommended for `stdbool.h` compatibility, though many modern GCC versions default to a suitable standard.)*

2.  **Run:**
    ```bash
    ./processor_sim
    ```
    The simulator will load instructions from `test4.txt` (or as specified in `main.c`) and print the simulation trace to the console.

## Test Program:

The primary test program used is `test4.txt`, which includes a variety of instructions and scenarios to test:
*   Immediate value loading (positive and negative).
*   Conditional branches (BEQZ - taken and not taken).
*   Register jumps (JR).
*   Arithmetic and logical operations leading to SREG changes.
*   Sequences designed to trigger data hazards (stalls) and control hazards (flushes).
*   Load and Store operations.
*   Circular shift operations.
*   HALT instruction.

## Project Structure:

*   `main.c`: Contains the full C source code for the processor simulator.
*   `test4.txt`: An example assembly language program.
*   `Project_Team_20_Report.md`: The detailed project report.
*   `README.md`: This file.

---
**Team Number:** 20
**Package:** 4 - Double McHarvard with cheese circular shifts