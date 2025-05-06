#include <stdio.h>
//#include <stdint.h> This gives access to the uint16_t type, which is a 16-bit unsigned integer type that i wanted to use for the instructions to ensure that the instructions are always 16 bits long.
//but honestly it's too complicated to use it for now, so i'll just use int and maybe circle back once i ensure functionality is solid.

#define MEMORY_SIZE 1026

//variables
int PC = 0;

//arrays
int MAIN_MEMORY[MEMORY_SIZE] = {
    //I used chatgpt to generate test instructions for debugging, bas please remember to remove them because i don't want to risk a cheating case.
    0x412A, // ADDI R1, 10
    0x4254, // ADDI R2, 20
    0x0301, // ADD R3, R1
    0x0302, // ADD R3, R2
    0x5305, // SUBI R3, 5
    0x7300, // ST R3, 0
    0x6400, // LD R4, 0
    0x4902, // BEQZ R4, 2
    0x4501, // ADDI R5, 1
    0x0614, // ADD R6, R4
    0xA000, // NOP
    0xB000  // HALT
};


//functions
int fetch_instruction() {
    if (PC < MEMORY_SIZE) {
        int instruction = MAIN_MEMORY[PC];
        PC++;
        return instruction;
    }
}

void decode_instruction(int instruction) {
    int opcode = 0;
    int operand1 = 0;
    int operand2 = 0;

    opcode = (instruction >> 12);
    operand1 = ( (unsigned) (instruction >> 6) & 0b000000111111);
    operand2 = ( (unsigned) instruction & 0b0000000000111111);

    printf("Decoded instruction: %i %i %i\n", opcode, operand1, operand2);
}


int main(){

    decode_instruction(fetch_instruction());
    return 0;
}