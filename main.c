#include <stdio.h>
//#include <stdint.h> This gives access to the uint16_t type, which is a 16-bit unsigned integer type that i wanted to use for the instructions to ensure that the instructions are always 16 bits long.
//but honestly it's too complicated to use it for now, so i'll just use int and maybe circle back once i ensure functionality is solid.

#define MEMORY_SIZE 1026

//variables


//arrays
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

int MAIN_MEMORY[MEMORY_SIZE] = {
    //I used chatgpt to generate test instructions for debugging, bas please remember to remove them because i don't want to risk a cheating case.
    0x412A,
    0x4254,
    0x0301,
    0x0302,
    0x5305,
    0x7300,
    0x6400,
    0x4902,
    0x4501,
    0x0614,
    0xA000,
    0xB000
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
    int r1 = 0;
    int r2 = 0;
    int r1data = 0;
    int r2data = 0;

    opcode = ((unsigned) instruction >> 12);
    r1 = ((instruction >> 6) & 0b000000111111);
    r2 = (instruction & 0b0000000000111111);

    r1data = REGISTER_FILE[r1];
    r2data = REGISTER_FILE[r2];

    printf("Decoded instruction: %i %i %i\n", opcode, r1, r2);
}

int sign_extend(int imm) {
    int sign_bit = (imm >> 5);
    if (sign_bit = 1) {
        return imm | 0b11000000;
    }
    else{
        return imm | 0b00000000;
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


//function is still missing the special flags
void execute_instruction(int opcode, int r1, int r2, int r1data, int r2data) {
    int imm = sign_extend(r2);
    int carry = 0;
    int r1_sign_bit = (r1data >> 7);
    int r2_sign_bit = (r2data >> 7);
    int result_sign_bit = 0;


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
            PC = PC + 1 + imm;
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
        PC = r1data || r2data;
        break;

        case 8: //shift left circular
        int left_part  = (r1data << imm);
        int right_part = (r1data >> (8 - imm));
        REGISTER_FILE[r1data] = (left_part | right_part);

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 9: //shift right circular
        int right_part = (r1data >> imm) & 0xFF;
        int left_part  = (r1data << (8 - imm)) & 0xFF;
        REGISTER_FILE[r1data] = (left_part | right_part);

        set_negative_flag(r1data);
        set_zero_flag(r1data);
        break;

        case 10: //load byte
        r1data = MAIN_MEMORY[imm];
        REGISTER_FILE[r1] = r1data;
        break;

        case 11: //store byte
        MAIN_MEMORY[imm] = r1data;
        break;
    }
}


int main(){

    decode_instruction(fetch_instruction());
    return 0;
}