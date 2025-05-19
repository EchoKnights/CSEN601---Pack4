# Computer Systems Architecture – CSEN601
## Processor Simulation Project

---

**Package Number:** 4
**Package Name:** Double McHarvard with cheese circular shifts

**Team Number:** 20
<!-- **Team Name:** [If you have a specific team name, add it here] -->

---

### Team Members:

*   **Name:** Hatem Soliman
    *   **ID:** 58-6188
    *   **Tutorial:** 25
*   **Name:** Aly Tamer
    *   **ID:** 58-7536
    *   **Tutorial:** 22
*   **Name:** Youssef Hesham
    *   **ID:** 58-14831
    *   **Tutorial:** 28
*   **Name:** Amr Baher
    *   **ID:** 58-16143
    *   **Tutorial:** 22
*   **Name:** Mariam Ayman
    *   **ID:** 58-13210
    *   **Tutorial:** 28
*   **Name:** Tony Makaryous
    *   **ID:** 58-7496
    *   **Tutorial:** 11
*   **Name:** Mohammed ElAzab
    *   **ID:** 58-26108
    *   **Tutorial:** 12

---

**Date of Submission:** [e.g., May 19, 2025]

---

## Table of Contents
1.  [Introduction](#introduction)
2.  [Processor Architecture (Package 4)](#processor-architecture-package-4)
    1.  [Memory Architecture](#memory-architecture)
    2.  [Registers](#registers)
    3.  [Instruction Set Architecture (ISA)](#instruction-set-architecture-isa)
    4.  [Datapath and Pipeline](#datapath-and-pipeline)
3.  [Implementation Details](#implementation-details)
    1.  [Overview of the C Simulation](#overview-of-the-c-simulation)
    2.  [Key Data Structures](#key-data-structures)
    3.  [Core Functions](#core-functions)
        1.  [Program Loading (`loadProgram`)](#program-loading-loadprogram)
        2.  [Instruction Fetch (`fetch_instruction` & IF stage logic)](#instruction-fetch-fetch_instruction--if-stage-logic)
        3.  [Instruction Decode (`decode_instruction` & ID stage logic)](#instruction-decode-decode_instruction--id-stage-logic)
        4.  [Instruction Execute (`execute_instruction` & EX stage logic)](#instruction-execute-execute_instruction--ex-stage-logic)
    4.  [Pipeline Simulation](#pipeline-simulation)
    5.  [Status Register (SREG) Handling](#status-register-sreg-handling)
    6.  [Control Hazard Handling](#control-hazard-handling)
    7.  [Data Hazard Policy](#data-hazard-policy)
4.  [Testing and Verification](#testing-and-verification)
    1.  [Test Program Design](#test-program-design)
    2.  [Verification Strategy](#verification-strategy)
    3.  [Sample Test Cases and Output](#sample-test-cases-and-output)
5.  [Results and Analysis](#results-and-analysis)
    1.  [Pipeline State Tracing](#pipeline-state-tracing)
    2.  [Cycle Count Verification](#cycle-count-verification)
    3.  [Observed Behavior for Specific Instructions/Scenarios](#observed-behavior-for-specific-instructionsscenarios)
6.  [Challenges Encountered (Optional)](#challenges-encountered-optional)
7.  [Conclusion](#conclusion)
8.  [Appendix (Optional)](#appendix-optional)
    1.  [Full Source Code for Test Programs](#full-source-code-for-test-programs)
    2.  [Detailed Register and Memory Dumps](#detailed-register-and-memory-dumps)

---

## 1. Introduction
*   Briefly introduce the project: simulating a processor.
*   State the chosen package: "Package 4: Double McHarvard with cheese circular shifts."
*   Outline the purpose of the report: to describe the design, implementation, and verification of the simulated processor.
*   Mention the programming language used (C).

## 2. Processor Architecture (Package 4)
This section should detail the specifications of Package 4 that your simulation adheres to.

### 2.1. Memory Architecture
*   **Type:** Harvard Architecture.
*   **Instruction Memory:**
    *   Size: 1024 \* 16 bits.
    *   Address Range: 0 to 1023.
    *   Word Addressable (1 word = 16 bits).
*   **Data Memory:**
    *   Size: 2048 \* 8 bits.
    *   Address Range: 0 to 2047.
    *   Word/Byte Addressable (1 word = 1 byte = 8 bits).

### 2.2. Registers
*   **General Purpose Registers (GPRs):**
    *   Count: 64 (R0 to R63).
    *   Size: 8 bits.
*   **Status Register (SREG):**
    *   Size: 8 bits.
    *   Flags (and their bit positions, if fixed):
        *   Carry Flag (C)
        *   Two’s Complement Overflow Flag (V)
        *   Negative Flag (N)
        *   Sign Flag (S)
        *   Zero Flag (Z)
    *   Specify how unused bits (Bits 7:5) are handled (kept cleared "0").
*   **Program Counter (PC):**
    *   Size: 16 bits.
    *   Function: Holds the address of the instruction to be fetched.

### 2.3. Instruction Set Architecture (ISA)
*   **Instruction Size:** 16 bits.
*   **Instruction Types:** 2 (as per the table in the project description).
    *   Describe the formats (e.g., R-type like `OP R1, R2` and I-type like `OP R1, Imm`).
    *   Show how the 16 bits are divided for opcode, registers, and immediate values for each type.
*   **Instruction Count:** 12.
*   **List of Instructions:**
    *   Provide a table or list: Mnemonic, Opcode, Format, Description of operation.
    *   (e.g., ADD, SUB, MUL, LDI, BEQZ, AND, OR, JR, SLC, SRC, LB, SB).
    *   Detail how immediate values are handled (e.g., 6-bit signed for LDI, BEQZ, LB, SB; 6-bit unsigned for SLC, SRC).
*   **SREG Flag Updates:**
    *   List which instructions affect which flags (C, V, N, S, Z).

### 2.4. Datapath and Pipeline
*   **Stages:** 3 (Instruction Fetch - IF, Instruction Decode - ID, Execute - EX).
*   **Describe each stage's responsibility:**
    *   IF: Fetches instruction, increments PC.
    *   ID: Decodes instruction, reads operands from GPRs.
    *   EX: Executes ALU operations, performs memory access (load/store), writes results back to GPRs.
*   **Pipeline Depth:** 3 instructions maximum running in parallel.
*   **Clock Cycle Formula:** 3 + ((n − 1) ∗ 1). Explain how your simulation adheres to this.

## 3. Implementation Details
Describe how your C code simulates the processor.

### 3.1. Overview of the C Simulation
*   Briefly describe the main components of your `main.c` file.
*   How the simulation loop works.

### 3.2. Key Data Structures
*   `INSTRUCTION_MEMORY[]`: How it's declared and used.
*   `DATA_MEMORY[]`: How it's declared and used.
*   `REGISTER_FILE[]`: How it's declared and used.
*   `SREG_t` (or however SREG is implemented): Structure and bitfields.
*   `Decodedinstruction_t`: Structure for holding decoded instruction parts (opcode, r1, r2, r1data, r2data, address).
*   Pipeline registers: `if_reg`, `id_reg`, `ex_reg` (and `if_reg_addr`).

### 3.3. Core Functions
Describe the purpose and key logic of:

#### 3.3.1. Program Loading (`loadProgram`)
*   Reading assembly from a text file.
*   Parsing mnemonics and operands.
*   Assembling instructions into 16-bit words.
*   Storing words into `INSTRUCTION_MEMORY`.
*   Handling comments, empty lines, and labels (if any, though not explicitly required by the parser description).
*   Initializing `program_length`.

#### 3.3.2. Instruction Fetch (`fetch_instruction` & IF stage logic)
*   How `if_reg` and `if_reg_addr` are updated.
*   How `PC` is used and incremented (or updated by branches/jumps).
*   Interaction with `INSTRUCTION_MEMORY`.
*   Handling fetching beyond `program_length` (inserting NOPs).

#### 3.3.3. Instruction Decode (`decode_instruction` & ID stage logic)
*   Taking `if_reg` and `if_reg_addr` as input.
*   Populating the `Decodedinstruction_t` structure for `id_reg`.
*   Extracting opcode, register numbers, and immediate values.
*   Reading operand values (`r1data`, `r2data`) from `REGISTER_FILE`.
*   Sign extension of immediate values.

#### 3.3.4. Instruction Execute (`execute_instruction` & EX stage logic)
*   Taking `ex_reg` (a `Decodedinstruction_t`) as input.
*   The main switch statement based on `opcode`.
*   Logic for each instruction:
    *   Arithmetic/Logical operations.
    *   Memory access for LB/SB (interaction with `DATA_MEMORY`).
    *   PC updates for BEQZ/JR.
    *   Register write-back to `REGISTER_FILE`.
    *   SREG flag updates for relevant instructions.

### 3.4. Pipeline Simulation
*   How instructions move from IF -> ID -> EX in the main loop.
*   How `cycles` are incremented.
*   How the stopping condition for the simulation is determined (e.g., `executed < program_length + pipeline_depth - 1`).

### 3.5. Status Register (SREG) Handling
*   How `SREG_t` is updated by arithmetic, logical, and shift instructions.
*   Detail the logic for setting each flag (C, V, N, S, Z) according to the project specifications.
    *   Carry: 9th bit of unsigned result.
    *   Overflow: Based on signs of operands and result.
    *   Negative: Based on MSB of result.
    *   Sign: N XOR V.
    *   Zero: Result is zero.

### 3.6. Control Hazard Handling
*   Explain the mechanism for `BEQZ` and `JR`.
*   When the branch/jump condition and target address are determined (in EX).
*   How `flush_pipeline` works:
    *   Setting `if_reg` and `id_reg.raw` to `NOP_INSTR`.
    *   Resetting `if_reg_addr`.
    *   Updating `PC` and `fetched` to the target address.
*   How this ensures the correct instruction is fetched in the cycle after the branch/jump instruction's EX stage.

### 3.7. Data Hazard Handling (RAW Hazards)
*   **Policy:** The simulator implements data hazard handling for Read-After-Write (RAW) hazards using a pipeline stall mechanism.
*   **Detection:** A RAW hazard is detected if an instruction in the Execute (EX) stage (`ex_reg`) writes to a general-purpose register (excluding R0) that is read as a source by the instruction currently in the Decode (ID) stage (`id_reg`).
    *   The detection logic checks if `ex_reg` is a "producer" instruction (ADD, SUB, MUL, LDI, AND, OR, SLC, SRC, LB) and its destination register (`ex_reg.r1`) matches either `id_reg.r1` (if used as a source) or `id_reg.r2` (if `id_reg` is an R-type instruction using R2 as a source).
*   **Stalling Mechanism:**
    *   If a RAW hazard is detected:
        *   The instruction in `ex_reg` (the producer) is allowed to complete its execution and write-back in the current cycle.
        *   A 1-cycle stall is introduced.
        *   For the next cycle:
            *   A NOP "bubble" is inserted into the `ex_reg`.
            *   The instruction in `id_reg` (the consumer) and the instruction in `if_reg` are effectively stalled (they are not advanced, and `PC`/`fetched` do not progress).
        *   The `total_stalled_cycles` counter is incremented.
    *   This stall ensures that when the consumer instruction in `id_reg` re-attempts decoding in the cycle after the stall, it reads the updated register value written by the producer.
*   **Impact on Performance:** Stalling introduces bubbles into the pipeline, increasing the total number of clock cycles required to execute a program compared to a pipeline without data hazard handling or with forwarding.

## 4. Testing and Verification

### 4.1. Test Program Design
*   Describe the assembly language test programs you wrote (e.g., `test4.txt`).
*   What specific instructions, sequences, or scenarios did you aim to test with each program?
    *   All individual instructions.
    *   SREG flag operations.
    *   Branch taken/not-taken paths.
    *   Jump register functionality.
    *   Memory load/store operations.
    *   Circular shifts with different amounts.
    *   Sequences that might expose control hazards.

### 4.2. Verification Strategy
*   How did you verify the correctness of your simulation?
    *   Manual tracing of pipeline states and register/memory values.
    *   Comparison against expected cycle counts.
    *   Checking `print_pipeline_state` output at each cycle.
    *   Checking final register and memory dumps.

### 4.3. Sample Test Cases and Output
*   Include snippets of key test assembly programs.
*   Show corresponding snippets of your simulator's output (pipeline state per cycle, register/memory updates).
*   Explain how the output demonstrates correct functionality for those test cases.
    *   Focus on a few illustrative examples, perhaps one for a branch, one for SREG changes, one for memory operations.

## 5. Results and Analysis

### 5.1. Pipeline State Tracing
*   Discuss the `print_pipeline_state` output format and its utility.
*   How it helps in visualizing the pipeline flow and debugging.

### 5.2. Cycle Count Verification
*   Show how the total cycles reported by your simulator match the formula `3 + (program_length - 1) * 1` for programs without branches.
*   Discuss how branches might affect the total cycle count (though the formula might not directly apply to programs with taken branches that flush instructions).

### 5.3. Observed Behavior for Specific Instructions/Scenarios
*   Discuss any interesting observations or confirmations of correct behavior.
    *   e.g., correct functioning of circular shifts.
    *   Correct SREG flag settings under various arithmetic conditions.
    *   Correct flushing and redirection on taken branches/jumps.

## 6. Challenges Encountered (Optional)
*   Briefly mention any significant challenges faced during design or implementation.
*   How these challenges were overcome. (e.g., debugging pipeline logic, correctly implementing sign extension, SREG flag logic).

## 7. Conclusion
*   Summarize what was achieved in the project.
*   Reiterate that the simulation for Package 4 was successfully implemented according to specifications, including control hazard handling.
*   Any final thoughts or potential future improvements (e.g., adding data hazard handling, more complex test cases).

## 8. Appendix (Optional)

### 8.1. Full Source Code for Test Programs
*   If your test assembly files are extensive, you can include their full content here.

### 8.2. Detailed Register and Memory Dumps
*   If relevant, provide more extensive final register/memory dumps for specific test programs that are too large for the main body of the report.

---