#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> // For bool, true, false
// #include <stdint.h> This gives access to the uint16_t type, which is a 16-bit unsigned integer type that i wanted to use for the instructions to ensure that the instructions are always 16 bits long.
// but honestly it's too complicated to use it for now, so i'll just use int and maybe circle back once i ensure functionality is solid.

#define INSTRUCTION_MEMORY_SIZE 1024
#define DATA_MEMORY_SIZE 2048
#define NOP_INSTR 0b1100000000000000  // nop instruction encoding, which just does nothing and acts as a filler.
#define HALT_INSTR 0b1101000000000000 // halt instruction encoding.

// variables
FILE *fptr;

int fetched = 0;
int executed = 0;
int cycles = 0;       // number of cycles that have passed.
int max_cycles = 100; // if i don't include this, the program runs forever
int program_length = 0;
int total_stalled_cycles = 0; // For data hazard stalls
int pipeline_depth = 3;       // aka how many stages are in the pipleline (In our case, 3 stages: fetch, decode, execute)
int if_reg = NOP_INSTR;       // raw 16-bit word in the fetch stage
int if_reg_addr = -1;         // Address of the instruction in if_reg

// arrays
typedef struct
{
  const char *mn;
  int opcode;
  int is_immediate;
} ISA; // the ISA table - format: (mnemonic (the actual assembly instruction), opcode, whether r2 is immediate or not(1 = yes, 0 = no)).

static const ISA ISATAB[] = {
    {"ADD", 0, 0},
    {"SUB", 1, 0},
    {"MUL", 2, 0},
    {"LDI", 3, 1},
    {"BEQZ", 4, 1},
    {"AND", 5, 0},
    {"OR", 6, 0},
    {"JR", 7, 0},
    {"SLC", 8, 1},
    {"SRC", 9, 1},
    {"LB", 10, 1},
    {"SB", 11, 1},
    {"NOP", 12, 0},
    {"HALT", 13, 0}};
#define ISA_LEN (sizeof ISATAB / sizeof ISATAB[0])

typedef struct
{
  int raw;     // the 16-bit word grabbed from memory.
  int address; // Address where this instruction was fetched from
  int opcode;
  int r1;
  int r2;
  int r1data;
  int r2data;
} Decodedinstruction_t;

Decodedinstruction_t id_reg = {.raw = NOP_INSTR, .address = -1}; // decoded word in the decoding stage
Decodedinstruction_t ex_reg = {.raw = NOP_INSTR, .address = -1}; // decoded word in the execution stage

int REGISTER_FILE[64]; // missing the special registers btw.

typedef struct
{
  unsigned Z : 1;      // zero flag
  unsigned S : 1;      // sign flag
  unsigned N : 1;      // negative flag
  unsigned V : 1;      // overflow flag
  unsigned C : 1;      // carry flag
  unsigned unused : 3; // bits 5-7
} SREG_t;

SREG_t SREG = {0};

int PC = 0; // program counter

int INSTRUCTION_MEMORY[INSTRUCTION_MEMORY_SIZE] = {0};

int DATA_MEMORY[DATA_MEMORY_SIZE]; // this is the data memory, which is initialized to 0 by default.

// functions
const ISA *get_instr_by_mnemonic(const char *mn)
{
  for (int i = 0; i < ISA_LEN; i++)
    if (strcmp(ISATAB[i].mn, mn) == 0)
      return &ISATAB[i];
  return NULL;
}

const ISA *get_instr_by_opcode(int opcode)
{
  for (int i = 0; i < ISA_LEN; i++)
    if (ISATAB[i].opcode == opcode)
      return &ISATAB[i];
  return NULL;
}

int sign_extend(int imm)
{
  if (imm & 0b00100000)
    return imm | ~0b00111111;
  else
    return imm & 0b00111111;
}

// this function is meant to parse immediate values, including negative and anything with a '#' prefix
int parse_immediate(const char *s)
{
  // skips the '#' prefix if it exists
  if (s[0] == '#')
    s++;
  // Handle negative values
  int val = atoi(s);
  // Mask to 6 bits (for 6-bit immediate field, two's complement)
  if (val < 0)
  {
    val = (val + 64) & 0x3F; // 6-bit two's complement
  }
  else
  {
    val = val & 0x3F;
  }
  return val;
}

void loadProgram(const char *fname, int start_addr)
{
  FILE *f = fopen(fname, "r");
  if (!f)
  {
    printf("Failed to open %s\n", fname);
    exit(1);
  }

  char buf[128]; // modified version of the template code from w3schools, but it's efficient and it works
  int instr_loaded = 0;
  int was_empty = (program_length == 0); // flag that checks if the memory was empty when loading the program

  while (fgets(buf, sizeof buf, f) && (start_addr + instr_loaded) < INSTRUCTION_MEMORY_SIZE)
  {
    // strip any comments so it doesn't brick the parser
    char *c = strpbrk(buf, ";");
    if (c)
      *c = '\0';

    // skip empty lines
    char *m = strtok(buf, " \t\r\n");
    if (!m)
      continue;

    // pulls the instruction by it's mnemonic and checks if it's valid
    const ISA *p = get_instr_by_mnemonic(m);
    if (!p)
    {
      printf("Unknown instruction: %s\n", m);
      continue;
    }

    int r1 = 0, r2 = 0;
    if (p->opcode != 12 && p->opcode != 13)
    {
      // R1
      char *t = strtok(NULL, " ,\t\n");
      if (!t)
        continue;
      r1 = atoi(t + 1);

      // R2
      t = strtok(NULL, " ,\t\n");
      if (!t)
        continue;
      if (p->is_immediate)
      {
        r2 = parse_immediate(t); // use helper for immediate values
      }
      else
      {
        r2 = (atoi(t + 1));
      }
    }

    // folds the assembly instruction into a 16-bit word to be added into memory
    int word = (p->opcode << 12) | (r1 << 6) | r2;
    INSTRUCTION_MEMORY[start_addr + instr_loaded] = word; // adds the word into memory
    instr_loaded++;                                       // increments to keep track of how many instructions were loaded
    if (p->opcode == 13)
      break; // ensures the loading process stops at HALT
  }
  fclose(f);

  // updates program_length
  if (start_addr + instr_loaded > program_length)
    program_length = start_addr + instr_loaded;

  // preps the pipeline if this is the first program loaded
  if (was_empty && instr_loaded > 0)
  {
    if_reg = INSTRUCTION_MEMORY[0];
    fetched = 1;
    id_reg.raw = NOP_INSTR;
    ex_reg.raw = NOP_INSTR;
  }
}

int fetch_instruction()
{
  if (PC < INSTRUCTION_MEMORY_SIZE)
  {
    int instruction = INSTRUCTION_MEMORY[PC];
    PC++;
    return instruction;
  }
}

Decodedinstruction_t decode_instruction(int instruction, int instruction_address)
{
  {
    Decodedinstruction_t i = {.raw = instruction, .address = instruction_address};
    i.opcode = ((unsigned)instruction >> 12);
    i.r1 = ((instruction >> 6) & 0b000000111111);
    i.r2 = (instruction & 0b0000000000111111);

    i.r1data = REGISTER_FILE[i.r1];
    i.r2data = REGISTER_FILE[i.r2];

    return i;
  }
}

void flush_pipeline(int new_pc)
{
  if_reg = NOP_INSTR;
  if_reg_addr = -1; // Mark address of NOP in IF as invalid/special
  id_reg = decode_instruction(NOP_INSTR, -1);
  fetched = new_pc;
  PC = new_pc;
}

// this function prints all instructions in the instruction in assembly format
void print_instructions()
{
  printf("Instruction Memory:\n");
  for (int i = 0; i < INSTRUCTION_MEMORY_SIZE; i++)
  {
    int word = INSTRUCTION_MEMORY[i];
    if (word != 0)
    {
      int opcode = (word >> 12) & 0xF;
      int r1 = (word >> 6) & 0x3F;
      int r2 = word & 0x3F;
      const ISA *isa = get_instr_by_opcode(opcode);
      if (isa)
      {
        if (opcode == 12 || opcode == 13)
        { // NOP or HALT
          printf("Instruction %d: %s\n", i, isa->mn);
        }
        else if (isa->is_immediate)
        {
          printf("Instruction %d: %s R%d, #%d\n", i, isa->mn, r1, (r2 & 0x20) ? (r2 | ~0x3F) : r2);
        }
        else
        {
          printf("Instruction %d: %s R%d, R%d\n", i, isa->mn, r1, r2);
        }
      }
      else
      {
        printf("Instruction %d: UNKNOWN (0x%04X)\n", i, word);
      }
    }
  }
  printf("\n");
}

// responsible for printing everything in the pipeline, like the current instruction in each stage, as well as register and memory updates.
void print_pipeline_state(int cycle, int if_reg, Decodedinstruction_t id_reg, Decodedinstruction_t ex_reg)
{
  static int prev_register_file[64] = {0};
  static int prev_data_memory[DATA_MEMORY_SIZE] = {0};
  static int first_call = 1;

  // prints the current fetched instruction in binary
  printf("Cycle %2d  |  IF ", cycle + 1);
  for (int i = 15; i >= 0; i--)
  {
    printf("%d", (if_reg >> i) & 1);
    if (i % 4 == 0 && i != 0)
      printf(" ");
  }
  // prints the current instruction per stage
  const ISA *if_isa = get_instr_by_opcode((if_reg >> 12) & 0xF);
  const ISA *id_isa = get_instr_by_opcode(id_reg.opcode);
  const ISA *ex_isa = get_instr_by_opcode(ex_reg.opcode);

  printf("  |  IF: %-5s", if_isa ? if_isa->mn : "UNK");
  printf("  |  ID: %-5s", id_isa ? id_isa->mn : "UNK");
  printf("  |  EX: %-5s", ex_isa ? ex_isa->mn : "UNK");

  // on first call, just copy the initial state for comparison
  // this is to ensure that the first call doesn't print garbage values (which it did, repeatedly)
  if (first_call)
  {
    memcpy(prev_register_file, REGISTER_FILE, sizeof(REGISTER_FILE));
    memcpy(prev_data_memory, DATA_MEMORY, sizeof(DATA_MEMORY));
    first_call = 0;
    printf("\n");
    return;
  }

  // cheks for register updates
  int reg_updated = 0;
  for (int i = 0; i < 64; i++)
  {
    if (REGISTER_FILE[i] != prev_register_file[i])
    {
      if (!reg_updated)
      {
        printf("  |  Register Updates:");
        reg_updated = 1;
      }
      printf(" R%d: %d -> %d;", i, prev_register_file[i], REGISTER_FILE[i]);
    }
  }

  // checks for memory updates
  int mem_updated = 0;
  for (int i = 0; i < DATA_MEMORY_SIZE; i++)
  {
    if (DATA_MEMORY[i] != prev_data_memory[i])
    {
      if (!mem_updated)
      {
        printf("  |  Memory Updates:");
        mem_updated = 1;
      }
      printf(" M[%d]: %d -> %d;", i, prev_data_memory[i], DATA_MEMORY[i]);
    }
  }

  // updates the previous state so it can be compared to the current state
  memcpy(prev_register_file, REGISTER_FILE, sizeof(REGISTER_FILE));
  memcpy(prev_data_memory, DATA_MEMORY, sizeof(DATA_MEMORY));

  printf("\n");
}

void set_carry_flag(int carry)
{
  if (carry == 1)
  {
    SREG.C = 1;
  }
  else
  {
    SREG.C = 0;
  }
}

void set_zero_flag(int r1data)
{
  if (r1data == 0)
  {
    SREG.Z = 1;
  }
  else
  {
    SREG.Z = 0;
  }
}

void set_negative_flag(int r1data)
{
  if (r1data < 0)
  {
    SREG.N = 1;
  }
  else
  {
    SREG.N = 0;
  }
}

void set_sign_flag(int Z, int N)
{
  SREG.S = Z ^ N;
}

bool execute_instruction(Decodedinstruction_t instr)
{
  int opcode = instr.opcode;
  int r1 = instr.r1;
  int r2 = instr.r2; // Raw 6-bit field: R2 index for R-types, immediate for I-types
  int r1data = instr.r1data;
  int r2data = instr.r2data; // Content of REG[R2] (for R-types)

  int imm_val_signed = sign_extend(instr.r2); // Sign-extended immediate value from r2 field
  int carry = 0;
  int r1_sign_bit = (r1data >> 7);
  int r2_sign_bit = (r2data >> 7);
  int result_sign_bit = 0;
  int left_part = 0;
  int right_part = 0;

  switch (opcode)
  {       // the switch case should be updated to reflect the actual opcodes.
  case 0: // add
    r1data = instr.r1data + instr.r2data;
    REGISTER_FILE[instr.r1] = r1data;

    // set the carry flag
    carry = r1data >> 8;
    set_carry_flag(carry);

    set_negative_flag(r1data);
    set_zero_flag(r1data);
    set_sign_flag(SREG.Z, SREG.N);

    // set the overflow flag
    result_sign_bit = (r1data >> 7);
    if (r1_sign_bit == r2_sign_bit && r1_sign_bit != result_sign_bit)
    {
      SREG.V = 1;
    }
    else
    {
      SREG.V = 0;
    }
    break;

  case 1: // sub
    r1data = instr.r1data - instr.r2data;
    REGISTER_FILE[instr.r1] = r1data;

    set_negative_flag(r1data);
    set_zero_flag(r1data);
    set_sign_flag(SREG.Z, SREG.N);

    // overflow flag
    result_sign_bit = (r1data >> 7);
    if (r1_sign_bit != r2_sign_bit && r2_sign_bit == result_sign_bit)
    {
      SREG.V = 1;
    }
    else
    {
      SREG.V = 0;
    }
    break;

  case 2: // mul
    r1data = instr.r1data * instr.r2data;
    REGISTER_FILE[instr.r1] = r1data;
    set_negative_flag(r1data);
    set_zero_flag(r1data);
    break;

  case 3:                                     // load immediate (LDI R1, Imm)
    REGISTER_FILE[instr.r1] = imm_val_signed; // Immediate is from instr.r2 (6-bit field), sign-extended
    break;

  case 4: // branch if equal zero (BEQZ R1, Imm)
    // R1 content is in instr.r1data
    // Immediate for offset is in instr.r2 (6-bit field)
    if (instr.r1data == 0)
    {
      // Target PC = (Address of BEQZ instruction) + 1 + sign_extended_IMM
      int pc_of_beqz_plus_1 = instr.address + 1;
      int target = pc_of_beqz_plus_1 + imm_val_signed;
      flush_pipeline(target);
      printf("Branch taken to address %d (from instruction at %d)\n", target, instr.address);
      return true;
    }
    break;

  case 5: // and (AND R1, R2)
    r1data = instr.r1data & instr.r2data;
    REGISTER_FILE[instr.r1] = r1data;
    set_negative_flag(r1data);
    set_zero_flag(r1data);
    break;

  case 6: // or (OR R1, R2)
    r1data = instr.r1data | instr.r2data;
    REGISTER_FILE[instr.r1] = r1data;
    set_negative_flag(r1data);
    set_zero_flag(r1data);
    break;

  case 7: // jump register (JR R1)
    // Target address is content of R[R1] (instr.r1data)
    flush_pipeline(instr.r1data);
    printf("Jumping to address %d (from R%d which holds %d, instruction at %d)\n", instr.r1data, instr.r1, instr.r1data, instr.address);
    return true;

  case 8: // shift left circular (SLC R1, Imm)
  {
    int shift_amount = instr.r2; // Immediate shift amount from instr.r2 (0-63)
    int effective_shift = shift_amount % 8;

    if (instr.r1data == 0)
    {
      REGISTER_FILE[instr.r1] = 0;
    }
    else if (effective_shift == 0)
    {
      REGISTER_FILE[instr.r1] = instr.r1data;
    }
    else
    {
      REGISTER_FILE[instr.r1] = ((instr.r1data << effective_shift) | (instr.r1data >> (8 - effective_shift))) & 0xFF;
    }
    set_negative_flag(REGISTER_FILE[instr.r1]);
    set_zero_flag(REGISTER_FILE[instr.r1]);
  }
  break;

  case 9: // shift right circular (SRC R1, Imm)
  {
    int shift_amount = instr.r2; // Immediate shift amount from instr.r2 (0-63)
    int effective_shift = shift_amount % 8;

    if (instr.r1data == 0)
    {
      REGISTER_FILE[instr.r1] = 0;
    }
    else if (effective_shift == 0)
    {
      REGISTER_FILE[instr.r1] = instr.r1data;
    }
    else
    {
      REGISTER_FILE[instr.r1] = ((instr.r1data >> effective_shift) | (instr.r1data << (8 - effective_shift))) & 0xFF;
    }
    set_negative_flag(REGISTER_FILE[instr.r1]);
    set_zero_flag(REGISTER_FILE[instr.r1]);
  }
  break;

  case 10: // load byte (LB R1, Imm)
    // Immediate from instr.r2 (sign-extended) is the memory address
    if (imm_val_signed >= 0 && imm_val_signed < DATA_MEMORY_SIZE)
    {
      REGISTER_FILE[instr.r1] = DATA_MEMORY[imm_val_signed];
    }
    else
    {
      printf("LB: Address %d out of bounds.\n", imm_val_signed);
      REGISTER_FILE[instr.r1] = 0; // Default behavior for out-of-bounds read
    }
    break;

  case 11: // store byte (SB R1, Imm)
    // R[R1] (instr.r1data) is stored at Mem[imm_val_signed]
    // Immediate from instr.r2 (sign-extended) is the memory address
    if (imm_val_signed >= 0 && imm_val_signed < DATA_MEMORY_SIZE)
    {
      DATA_MEMORY[imm_val_signed] = instr.r1data;
    }
    else
    {
      printf("SB: Address %d out of bounds.\n", imm_val_signed);
    }
    break;

  case 12: // nop
    // do nothing
    break;

  case 13: // halt
    printf("HALT instruction executed.\n");
  }
  return false;
}

int main()
{
  // Initialize default NOPs for pipeline registers that carry Decodedinstruction_t
  id_reg = decode_instruction(NOP_INSTR, -1);
  ex_reg = decode_instruction(NOP_INSTR, -1);
  if_reg_addr = -1; // Address of the instruction in if_reg (if_reg itself is just an int)

  loadProgram("test4.txt", 0); // load the program from the file.
                               // This sets global: program_length
                               // and if program loaded: if_reg=MEM[0], fetched=1, id_reg.raw=NOP, ex_reg.raw=NOP

  // Sync/initialize pipeline state after loadProgram
  cycles = 0;
  PC = 0; // PC generally starts at 0 or is set by jumps

  if (program_length > 0)
  {
    // loadProgram already primed if_reg with MEM[0] and fetched to 1
    // So, if_reg_addr should correspond to the address of MEM[0], which is 0.
    if_reg_addr = 0;
    // id_reg and ex_reg were set to NOP by loadProgram (their .raw field)
    // and our earlier decode_instruction(NOP_INSTR, -1) call set them up fully.
  }
  else
  {
    // No program loaded, ensure all pipeline stages are NOP and consistent
    if_reg = NOP_INSTR;
    if_reg_addr = -1;
    fetched = 0;
    // id_reg and ex_reg are already full NOPs
  }

  print_instructions(); // prints the instructions in the instruction memory in assembly format

  // print the initial pipeline state
  print_pipeline_state(cycles, if_reg, id_reg, ex_reg);

  // ensures that the program doesn't run forever.
  while (executed < program_length + pipeline_depth - 1 + total_stalled_cycles) // Adjusted for stalls
  {
    if (cycles >= max_cycles)
    {
      printf("maximum cycles reached.\n");
      break;
    }

    // Capture pipeline state at the beginning of the cycle for decision making
    Decodedinstruction_t current_cycle_ex_reg = ex_reg;
    Decodedinstruction_t current_cycle_id_reg = id_reg;
    int current_cycle_if_reg = if_reg;
    int current_cycle_if_reg_addr = if_reg_addr;

    print_pipeline_state(cycles, current_cycle_if_reg, current_cycle_id_reg, current_cycle_ex_reg);

    bool stall_this_cycle = false;
    bool branch_taken_or_jumped_in_ex = false;
    if (current_cycle_ex_reg.raw != NOP_INSTR && current_cycle_id_reg.raw != NOP_INSTR)
    {
      const ISA *ex_isa_info = get_instr_by_opcode(current_cycle_ex_reg.opcode);
      const ISA *id_isa_info = get_instr_by_opcode(current_cycle_id_reg.opcode);

      bool ex_is_producer = ex_isa_info &&
                            (current_cycle_ex_reg.opcode == 0 || current_cycle_ex_reg.opcode == 1 || current_cycle_ex_reg.opcode == 2 || // ADD, SUB, MUL
                             current_cycle_ex_reg.opcode == 3 ||                                                                         // LDI
                             current_cycle_ex_reg.opcode == 5 || current_cycle_ex_reg.opcode == 6 ||                                     // AND, OR
                             current_cycle_ex_reg.opcode == 8 || current_cycle_ex_reg.opcode == 9 ||                                     // SLC, SRC
                             current_cycle_ex_reg.opcode == 10);                                                                         // LB

      if (ex_is_producer && current_cycle_ex_reg.r1 != 0)
      { // R0 is not a hazard source as it cannot be overwritten
        int producer_dest_reg = current_cycle_ex_reg.r1;

        // Check RAW for id_reg.r1 source
        // LDI (opcode 3) does not use its R1 field as a source operand for its main operation.
        bool id_reads_r1_as_source = id_isa_info && (current_cycle_id_reg.opcode != 3);
        if (id_reads_r1_as_source && current_cycle_id_reg.r1 == producer_dest_reg)
        {
          stall_this_cycle = true;
        }

        // Check RAW for id_reg.r2 source (if R-type for r2)
        if (!stall_this_cycle && id_isa_info && !id_isa_info->is_immediate)
        {
          // R-type instructions using R2 as a data source: ADD, SUB, MUL, AND, OR
          bool id_reads_r2_as_reg_data_source = (current_cycle_id_reg.opcode == 0 || current_cycle_id_reg.opcode == 1 ||
                                                 current_cycle_id_reg.opcode == 2 || current_cycle_id_reg.opcode == 5 ||
                                                 current_cycle_id_reg.opcode == 6);
          if (id_reads_r2_as_reg_data_source && current_cycle_id_reg.r2 == producer_dest_reg)
          {
            stall_this_cycle = true;
          }
        }
      }
    }

    // Execute instruction in EX stage (happens regardless of stall for the producer)
    if (current_cycle_ex_reg.raw != NOP_INSTR)
    {
      branch_taken_or_jumped_in_ex = execute_instruction(current_cycle_ex_reg);
    }
    // Increment executed count if a valid instruction (not NOP) finished EX, unless it's HALT (HALT stops further execution counting implicitly by breaking)
    if (current_cycle_ex_reg.raw != NOP_INSTR && current_cycle_ex_reg.opcode != 13)
    {
      executed++;
    }

    cycles++; // Increment cycle count

    if (stall_this_cycle)
    {
      total_stalled_cycles++;
      printf("Data hazard: Stall inserted at end of cycle %d (entering cycle %d)\n", cycles - 1, cycles);
      ex_reg = decode_instruction(NOP_INSTR, -1); // Bubble into EX
                                                  // id_reg, if_reg, fetched are NOT advanced. They hold their values for the next cycle's start.
    }
    else if (branch_taken_or_jumped_in_ex)
    {
      printf("Control hazard: Pipeline flushed at end of cycle %d (entering cycle %d)\n", cycles - 1, cycles);
      // 'fetched' was updated by flush_pipeline to the target address.
      // Global 'if_reg' and 'id_reg' were set to NOP by flush_pipeline.
      // For the NEXT cycle:
      ex_reg = decode_instruction(NOP_INSTR, -1); // Instruction from ID (now flushed) becomes NOP
      id_reg = decode_instruction(NOP_INSTR, -1); // Instruction from IF (now flushed) becomes NOP

      // Fetch the target instruction into IF for the NEXT cycle
      if_reg_addr = fetched;
      if (fetched < program_length)
      {
        if_reg = INSTRUCTION_MEMORY[fetched++];
      }
      else
      {
        if_reg = NOP_INSTR;
        if_reg_addr = -1;
      }
    }
    else
    {
      // NORMAL pipeline progression for NEXT cycle
      ex_reg = current_cycle_id_reg;
      id_reg = decode_instruction(current_cycle_if_reg, current_cycle_if_reg_addr);

      // Fetch next instruction for IF stage for NEXT cycle
      // This 'fetched' is the one for the *next* instruction to bring into if_reg
      if_reg_addr = fetched;
      if (fetched < program_length)
      {
        if_reg = INSTRUCTION_MEMORY[fetched++];
      }
      else
      {
        if_reg = NOP_INSTR;
        if_reg_addr = -1;
      }
    }

    // Check for HALT condition based on the instruction that just FINISHED EX stage
    if (current_cycle_ex_reg.opcode == 13)
    { // HALT
      printf("HALT processed in EX at end of cycle %d. Simulation ending.\n", cycles - 1);
      // Final print of pipeline state before breaking
      print_pipeline_state(cycles, if_reg, id_reg, ex_reg);
      break;
    }
  }

  printf("Total cycles = %d (spec for no stalls/branches = %d + (%d-1)*1 = %d)\n", cycles, 3, program_length, 3 + (program_length - 1));
  printf("Total stalled cycles due to data hazards = %d\n", total_stalled_cycles);

  return 0;
}