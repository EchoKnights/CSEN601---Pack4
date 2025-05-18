#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdint.h> This gives access to the uint16_t type, which is a 16-bit unsigned integer type that i wanted to use for the instructions to ensure that the instructions are always 16 bits long.
//but honestly it's too complicated to use it for now, so i'll just use int and maybe circle back once i ensure functionality is solid.

#define INSTRUCTION_MEMORY_SIZE 1024
#define DATA_MEMORY_SIZE 2048
#define NOP_INSTR 0b1100000000000000  //nop instruction encoding, which just does nothing and acts as a filler.
#define HALT_INSTR 0b1101000000000000 //halt instruction encoding.

//variables
FILE *fptr; 

int fetched = 0;
int executed = 0;
int cycles = 0; //number of cycles that have passed.
int max_cycles = 100; //if i don't include this, the program runs forever     
int program_length = 0;
int pipeline_depth = 3;
int if_reg = NOP_INSTR; //raw 16-bit word in the fetch stage


//arrays
typedef struct { const char *mn; int opcode; int is_immediate; } ISA; //the ISA table - format: (mnemonic (the actual assembly instruction), opcode, whether r2 is immediate or not(1 = yes, 0 = no)).

static const ISA ISATAB[] = {
    {"ADD",  0, 0}, 
    {"SUB",  1, 0}, 
    {"MUL", 2, 0},
    {"LDI",  3, 1}, 
    {"BEQZ", 4, 1},
    {"AND",  5, 0}, 
    {"OR",  6, 0},
    {"JR",   7, 0},
    {"SLC",  8, 1}, 
    {"SRC", 9, 1},
    {"LB",  10, 1}, 
    {"SB", 11, 1},
    {"NOP", 12, 0}, 
    {"HALT",13,0}
};
#define ISA_LEN (sizeof ISATAB/ sizeof ISATAB[0]) 

typedef struct {
    int raw; //the 16-bit word grabbed from memory.
    int opcode;
    int r1;
    int r2;
    int r1data;
    int r2data;
} Decodedinstruction_t;

Decodedinstruction_t id_reg = {.raw = NOP_INSTR}; //decoded word in the decoding stage
Decodedinstruction_t ex_reg = {.raw = NOP_INSTR}; //decoded word in the execution stage


int REGISTER_FILE[64]; //missing the special registers btw.

typedef struct {
    unsigned Z : 1; //zero flag
    unsigned S : 1; //sign flag
    unsigned N : 1; //negative flag
    unsigned V : 1; //overflow flag
    unsigned C : 1; //carry flag
    unsigned unused: 3; //bits 5-7
} SREG_t;

SREG_t SREG = {0}; 

int PC = 0; //program counter

int INSTRUCTION_MEMORY[INSTRUCTION_MEMORY_SIZE] = {0};

int DATA_MEMORY[DATA_MEMORY_SIZE]; //this is the data memory, which is initialized to 0 by default.


//functions
const ISA *get_instr_by_mnemonic(const char *mn) {
    for (int i = 0; i < ISA_LEN; i++)
        if (strcmp(ISATAB[i].mn, mn) == 0)
            return &ISATAB[i];
    return NULL;
}

const ISA *get_instr_by_opcode(int opcode) {
    for (int i = 0; i < ISA_LEN; i++)
        if (ISATAB[i].opcode == opcode)
            return &ISATAB[i];
    return NULL;
}

void loadProgram(const char *fname, int start_addr) {
    FILE *f = fopen(fname,"r");
    if (!f) {
        printf("Failed to open %s\n", fname);
        exit(1);
    }

    char buf[128];
    int instr_loaded = 0;
    int was_empty = (program_length == 0);

    while(fgets(buf,sizeof buf,f) && (start_addr + instr_loaded) < INSTRUCTION_MEMORY_SIZE) {
        //strip any comments so it doesn't brick the parser
        char *c = strpbrk(buf,";#");
        if(c) *c = '\0';

        //skip empty lines
        char *m = strtok(buf," \t\r\n");
        if (!m) continue;

        const ISA *p = get_instr_by_mnemonic(m);
        if (!p) {
            printf("Unknown instruction: %s\n", m);
            continue;
        }

        int r1=0, r2=0;
        if(p->opcode!=12 && p->opcode!=13){ 
            //R1
            char *t = strtok(NULL," ,\t\n");  
            if (!t) continue;
            r1 = atoi(t+1);

            //R2 
            t = strtok(NULL," ,\t\n");
            if (!t) continue;
            if (p->is_immediate){
                r2 = (atoi(t)); 
            }
            else{ 
                r2 = (atoi(t+1));
            }
        }

        int word = (p->opcode<<12) | (r1<<6) | r2;
        INSTRUCTION_MEMORY[start_addr + instr_loaded] = word;
        instr_loaded++;
        if(p->opcode==13) break; //stop at HALT
    }
    fclose(f);

    //updates program_length
    if (start_addr + instr_loaded > program_length)
        program_length = start_addr + instr_loaded;

    //preps the pipeline if this is the first program loaded
    if (was_empty && instr_loaded > 0) {
        if_reg = INSTRUCTION_MEMORY[0];
        fetched = 1;
        id_reg.raw = NOP_INSTR;
        ex_reg.raw = NOP_INSTR;
    }
}

int fetch_instruction() {
    if (PC < INSTRUCTION_MEMORY_SIZE) {
        int instruction = INSTRUCTION_MEMORY[PC];
        PC++;
        return instruction;
    }
}

Decodedinstruction_t decode_instruction(int instruction) {
    {   
        Decodedinstruction_t i = {.raw = instruction};
        i.opcode = ((unsigned) instruction >> 12);
        i.r1 = ((instruction >> 6) & 0b000000111111);
        i.r2 = (instruction & 0b0000000000111111);
    
        i.r1data = REGISTER_FILE[i.r1];
        i.r2data = REGISTER_FILE[i.r2];

        return i;
    }
}


int sign_extend(int imm) {
    int sign_bit = (imm >> 5) & 1;
    if (sign_bit == 1) {
        return imm | 0b11000000;
    }
    else{
        return imm & 0b00111111;
    }
}

void set_carry_flag(int carry) {
    if (carry == 1) {
        SREG.C = 1;
    } 
    else{
        SREG.C = 0;
    }
}

void set_zero_flag(int r1data) {
    if (r1data == 0) {
        SREG.Z = 1;
    } else {
        SREG.Z = 0;
    }
}

void set_negative_flag(int r1data) {
    if (r1data < 0) {
        SREG.N = 1;
    } else {
        SREG.N = 0;
    }
}

void set_sign_flag(int Z, int N) {
    SREG.S = Z ^ N;
}

void execute_instruction(int opcode, int r1, int r2, int r1data, int r2data) {
    int imm = sign_extend(r2);
    int carry = 0;
    int r1_sign_bit = (r1data >> 7);
    int r2_sign_bit = (r2data >> 7);
    int result_sign_bit = 0;
    int left_part = 0;
    int right_part = 0;

    switch (opcode) { //the switch case should be updated to reflect the actual opcodes.
        case 0: //add
        r1data = r1data + r2data;
        REGISTER_FILE[r1] = r1data;

        //set the carry flag
        carry = r1data >> 8;
        set_carry_flag(carry);

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        set_sign_flag(SREG.Z, SREG.N);

        //set the overflow flag
        result_sign_bit = (r1data >> 7);
        if (r1_sign_bit == r2_sign_bit && r1_sign_bit != result_sign_bit) {
            SREG.V = 1;
        } 
        else{
            SREG.V = 0;
        }
        break;

        case 1: //sub
        r1data = r1data - r2data;
        REGISTER_FILE[r1] = r1data;

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        set_sign_flag(SREG.Z, SREG.N);

        //overflow flag
        result_sign_bit = (r1data >> 7);
        if (r1_sign_bit != r2_sign_bit && r2_sign_bit == result_sign_bit) {
            SREG.V = 1;
        } 
        else{
            SREG.V = 0;
        }
        break;

        case 2: //mul
        r1data = r1data * r2data;
        REGISTER_FILE[r1] = r1data;
        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 3: //load immediate
        r1data = imm;
        REGISTER_FILE[r1] = r1data;
        break;

        case 4: //branch if equal zero
        if(r1data == 0) {
            PC += sign_extend(r2);
        }
        break;

        case 5: //and
        r1data = r1data & r2data;
        REGISTER_FILE[r1] = r1data;
        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 6: //or
        r1data = r1data | r2data;
        REGISTER_FILE[r1] = r1data;
        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 7: //jump register
        PC = r1data | r2data;
        break;

        case 8: //shift left circular
        left_part  = (r1data << DATA_MEMORY[imm]);
        right_part = (r1data >> (8 - DATA_MEMORY[imm]));
        REGISTER_FILE[r1] = (left_part | right_part);

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 9: //shift right circular
        right_part = (r1data >> imm) & 0xFF;
        left_part  = (r1data << (8 - imm)) & 0xFF;
        REGISTER_FILE[r1] = (left_part | right_part);

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 10: //load byte
        r1data = DATA_MEMORY[imm];
        REGISTER_FILE[r1] = r1data;
        break;

        case 11: //store byte
        DATA_MEMORY[imm] = r1data;
        break;
    }
}

int main(){
    loadProgram("test.txt", 0); //load the program from the file.
    id_reg = decode_instruction(NOP_INSTR);
    ex_reg = decode_instruction(NOP_INSTR);

    //ensures that the program doesn't run forever.
    while (executed < program_length + pipeline_depth - 1){
        if (cycles >= max_cycles) {            
            printf("maximum cycles reached.\n"); 
            break;   
        }

        // Print pipeline state BEFORE updating
        printf("Cycle %2d  |  IF ", cycles + 1);
        for (int i = 15; i >= 0; i--) {
            printf("%d", (if_reg >> i) & 1);
            if (i % 4 == 0 && i != 0) printf(" ");
        }
        printf("  |  ID op %2d  |  EX op %2d\n", id_reg.opcode, ex_reg.opcode);

        cycles++;
        ex_reg = id_reg;

        //shift the instruction inside the fetch slot to the decode slot.
        id_reg = decode_instruction(if_reg);

        //start the fetch again.
        if (fetched < program_length)
            if_reg = INSTRUCTION_MEMORY[fetched++];
        else
            if_reg = NOP_INSTR;

        //execute
        if (ex_reg.raw != NOP_INSTR) {
            execute_instruction(ex_reg.opcode, ex_reg.r1, ex_reg.r2, ex_reg.r1data, ex_reg.r2data);
        }
        executed++;
    }

    printf("Total cycles = %d (spec = 3 + (%d-1)×1 = %d)\n", cycles, program_length, 3 + (program_length-1));
    
    return 0;
}