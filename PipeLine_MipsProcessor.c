#define _CRT_SRCURE_NO_WARNINGS
#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define REGSIZE 32 // Register size
#define MEMSIZE 0x1000001 // Memory size
unsigned int InstructionMem[MEMSIZE];
unsigned int DataMem[MEMSIZE] = { 0 };
unsigned Regs[REGSIZE];

const char RegName[REGSIZE][6] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
}; // Register Name


typedef struct IF_ID_REG {
    unsigned int instruction;
    unsigned rs;
    unsigned rt;
    unsigned op;
    unsigned rd;
    unsigned funct;
    unsigned shamt;
}IF_ID_Reg;

typedef struct ID_EX_REG {
    unsigned int instruction;
    int RegDst;
    int AluSrc;
    int MemToReg;
    int RegWrite;
    int MemRead;
    int MemWrite;
    int Branch;
    int Jump;
    int AluOp[2];
    unsigned data1;
    unsigned data2;
    unsigned extend_value;
    unsigned rt;
    unsigned rd;
    unsigned op;
    unsigned shamt;
    unsigned funct;
    unsigned rs;
    char type;

}ID_EX_Reg;
typedef struct EX_MEM_REG {
    unsigned int instruction;
    int MemToReg;
    int RegWrite;
    int MemRead;
    int MemWrite;
    unsigned AluResult;
    unsigned rt;
    unsigned rd;
    unsigned op;
    unsigned funct;
    unsigned rs;
    unsigned extend_value;
    int Branch;
    int Jump;
    char type;

}EX_MEM_Reg;
typedef struct MEM_WB_REG {
    unsigned int instruction;
    unsigned memData;
    unsigned op;
    unsigned funct;
    unsigned AluResult;
    unsigned rt;
    unsigned rd;
    int RegWrite;
    unsigned rs;
    unsigned extend_value;
    int Branch;
    int Jump;
    char type;
}MEM_WB_Reg;

typedef struct writeBack {
    int branch;
    unsigned AluResult;
    unsigned rs;
    unsigned rt;
    unsigned rd;
    unsigned funct;
    unsigned extend_value;
    unsigned memData;
    unsigned op;
    int Jump;
    int RegWrite;
}WBPrint;

typedef struct ContorlSignals {
    int RegDst;
    int AluSrc;
    int MemToReg;
    int RegWrite;
    int MemRead;
    int MemWrite;
    int Branch;
    int Jump;
    int AluOp[2];
}Signal; // Control Signal

typedef struct instruction {
    unsigned op; //31-26
    unsigned rs; //25-21
    unsigned rt; //20-16
    unsigned rd; //15-11
    unsigned shamt; //10-6
    unsigned funct; //5-0
    unsigned offset; //15-0
    unsigned j; //25-0
    unsigned extend_value;

}Instruction; // Decode Instruction

int WB_stall = 0;
int MEM_stall = 0;
int EX_stall = 0;
int ID_stall = 0;
int IF_stall = 0;
int jr_stall = 0;
unsigned pc = 0; // PC count
int stall_cycle = 0;
int clockCycle = 1;
int ItypeCount = 0;
int RtypeCount = 0;
int JtypeCount = 0;
int MemoryCount = 0;

void writebackStage(MEM_WB_Reg* MEM_WB_reg, WBPrint* WB_print_data);
void memoryStage(EX_MEM_Reg* EX_MEM_reg, MEM_WB_Reg* MEM_WB_reg, ID_EX_Reg* ID_EX_reg);
void executeStage(EX_MEM_Reg* EX_MEM_reg, ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg);
void decodeStage(MEM_WB_Reg* MEM_WB_reg, EX_MEM_Reg* EX_MEM_reg, ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg, Instruction* instruction, Signal* signal);
void fetchStage(ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg);
void printResult(IF_ID_Reg* IF_ID_reg, ID_EX_Reg* ID_EX_reg, EX_MEM_Reg* EX_MEM_reg, MEM_WB_Reg* MEM_WB_reg, WBPrint* WB_print_data);

int main() {

    FILE* input_file;
    input_file = fopen("input.bin", "rb");

    if (input_file == NULL) {
        perror("Fail");
        exit(EXIT_FAILURE);
    }

    fread(InstructionMem, sizeof(unsigned int), MEMSIZE, input_file);
    fclose(input_file); 

    for (int i = 0; i < 29; i++) {
        Regs[i] = 0;
    }


    Regs[31] = 0xFFFFFFFF;
    Regs[30] = 0;
    Regs[29] = 0x1000000;
    for (int i = 0; i < MEMSIZE; i++) {
        unsigned int num =
            ((InstructionMem[i] >> 24) & 0xff) |
            ((InstructionMem[i] << 8) & 0xff0000) |
            ((InstructionMem[i] >> 8) & 0xff00) |
            ((InstructionMem[i] << 24) & 0xff000000);
        InstructionMem[i] = num;
    }
    IF_ID_Reg IF_ID_reg = { 0 };
    ID_EX_Reg ID_EX_reg = { 0 };
    EX_MEM_Reg EX_MEM_reg = { 0 };
    MEM_WB_Reg MEM_WB_reg = { 0 };
    Instruction instruction = { 0 };
    Signal signal = { 0 };
    WBPrint WB_print_data = { 0 };

    while (pc != 0xFFFFFFFF) {
        if (jr_stall != 1) {
            IF_stall = 0;
        }
        if (WB_stall != 1 && clockCycle >= 5) {
            writebackStage(&MEM_WB_reg, &WB_print_data);
        }
        if (MEM_stall != 1 && clockCycle >= 4) {
            memoryStage(&EX_MEM_reg, &MEM_WB_reg, &ID_EX_reg);
        }
        if (EX_stall != 1 && clockCycle >= 3) {
            executeStage(&EX_MEM_reg, &ID_EX_reg, &IF_ID_reg);
        }
        if (ID_stall != 1 && clockCycle >= 2) {
            decodeStage(&MEM_WB_reg, &EX_MEM_reg, &ID_EX_reg, &IF_ID_reg, &instruction, &signal);
        }
        if (IF_stall != 1) {
            fetchStage(&ID_EX_reg, &IF_ID_reg);
        }
        printResult(&IF_ID_reg, &ID_EX_reg, &EX_MEM_reg, &MEM_WB_reg, &WB_print_data);


    }
    printf("\n\n");
    printf("	  **RESULT**	\n");
    printf("Result Value = %d   ", Regs[2]);
    printf("Total Cycle = %d\n", clockCycle - 1);
    printf("R Type Instruction = %d\n", RtypeCount);
    printf("I Type Instruction = %d\n", ItypeCount);
    printf("J type Instruction = %d\n", JtypeCount);
    printf("Memory Count = %d\n", MemoryCount);

    return 0;
}

void fetchStage(ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg) {

    unsigned int instruction = 0;
    unsigned bit6 = 0x0000003f; //6bit total;
    unsigned bit5 = 0x0000001f;
    if (pc == 0xFFFFFFFF) {
        return;
    }
    instruction = InstructionMem[pc / 4];
    pc += 4;

    IF_ID_reg->instruction = instruction;
    IF_ID_reg->op = (instruction >> 26) & bit6;
    IF_ID_reg->rs = (instruction >> 21) & bit5;
    IF_ID_reg->rt = (instruction >> 16) & bit5;
    IF_ID_reg->rd = (instruction >> 11) & bit5;
    IF_ID_reg->shamt = (instruction >> 6) & bit5;
    IF_ID_reg->funct = instruction & bit6;

    //lw stall
    if (ID_EX_reg->MemRead == 1) {
        if ((ID_EX_reg->rt == IF_ID_reg->rs) || (ID_EX_reg->rt == IF_ID_reg->rt)) {
            pc = pc - 4;
            return;
        }
    }
    if (ID_EX_reg->op == 0 && ID_EX_reg->funct == 8) {
        jr_stall = 1;
        IF_stall = 1;
    }
}
void decodeStage(MEM_WB_Reg* MEM_WB_reg, EX_MEM_Reg* EX_MEM_reg, ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg, Instruction* instruction, Signal* signal) {

    if (pc == 0xFFFFFFFF) {
        return;
    }
    unsigned int n = IF_ID_reg->instruction;
    //decode
    unsigned bit6 = 0x0000003f; //6bit total;
    unsigned bit5 = 0x0000001f; //5 bits total
    unsigned bit16 = 0x0000ffff; //16 bits total
    unsigned bit26 = 0x03ffffff; //26 bits total
    instruction->op = (n >> 26) & bit6;
    instruction->rs = (n >> 21) & bit5;
    instruction->rt = (n >> 16) & bit5;
    instruction->rd = (n >> 11) & bit5;
    instruction->shamt = (n >> 6) & bit5;
    instruction->funct = n & bit6;
    instruction->offset = n & bit16;
    instruction->j = n & bit26;

    //sign extend
    unsigned extend = 0xFFFF0000;
    unsigned Negative = instruction->offset >> 15;
    if (Negative == 1)
        instruction->extend_value = instruction->offset | extend;
    else
        instruction->extend_value = instruction->offset & 0x0000ffff;
    //control
    if (instruction->op == 0) {
        signal->AluOp[0] = 1;
        signal->AluOp[1] = 0;
    } //R-type
    else if (instruction->op == 4 || instruction->op == 5) {
        signal->AluOp[0] = 0;
        signal->AluOp[1] = 1;
    } //I-type branch
    else if (instruction->op == 2 || instruction->op == 3) {
        signal->AluOp[0] = 1;
        signal->AluOp[1] = 1;
        ID_EX_reg->type = 'J';
        JtypeCount++;
        signal->RegDst = 0;
        signal->AluSrc = 0;
        signal->MemToReg = 0;
        signal->RegWrite = 0;
        signal->MemRead = 0;
        signal->MemWrite = 0;
        signal->Branch = 0;
        signal->Jump = 1;

    }
    else {
        signal->AluOp[0] = 0;
        signal->AluOp[1] = 0;
    } //I-type
    if (signal->AluOp[0] == 0 && signal->AluOp[1] == 0) {
        ID_EX_reg->type = 'I';
        ItypeCount++; //I-type
        if (instruction->op == 35) {
            signal->RegDst = 0;
            signal->AluSrc = 1;
            signal->MemToReg = 1;
            signal->RegWrite = 1;
            signal->MemRead = 1;
            signal->MemWrite = 0;
            signal->Branch = 0;
            signal->Jump = 0;
        } // LW
        else if (instruction->op == 43) {
            signal->RegDst = 0;
            signal->AluSrc = 1;
            signal->MemRead = 0;
            signal->RegWrite = 0;
            signal->MemToReg = 0;
            signal->MemWrite = 1;
            signal->Branch = 0;
            signal->Jump = 0;
        } //SW
        else {
            signal->RegDst = 0;
            signal->AluSrc = 1;
            signal->MemToReg = 0;
            signal->RegWrite = 1;
            signal->MemRead = 0;
            signal->MemWrite = 0;
            signal->Branch = 0;
            signal->Jump = 0;
        } //addi , slti
    }
    else if (signal->AluOp[0] == 1 && signal->AluOp[1] == 0) {
        ID_EX_reg->type = 'R';
        RtypeCount++;
        if (n == 0) {
            ID_EX_reg->type = 'X';
            signal->RegDst = 0;
            signal->AluSrc = 0;
            signal->MemToReg = 0;
            signal->RegWrite = 0;
            signal->MemRead = 0;
            signal->MemWrite = 0;
            signal->Branch = 0;
            signal->Jump = 0;
            RtypeCount--;
        }
        else
        {
            signal->RegDst = 1;
            signal->AluSrc = 0;
            signal->MemToReg = 0;
            signal->RegWrite = 1;
            signal->MemRead = 0;
            signal->MemWrite = 0;
            signal->Branch = 0;
            signal->Jump = 0;
        }
    }  //r-type
    else if (signal->AluOp[0] == 0 && signal->AluOp[1] == 1) {
        ID_EX_reg->type = 'I';
        ItypeCount++;
        signal->RegDst = 0;
        signal->AluSrc = 0;
        signal->MemToReg = 0;
        signal->RegWrite = 0;
        signal->MemRead = 0;
        signal->MemWrite = 0;
        signal->Branch = 1;
        signal->Jump = 0;
    } //branch

    //read register
    ID_EX_reg->data1 = Regs[instruction->rs];
    if (signal->AluSrc == 1) {
        ID_EX_reg->data2 = instruction->extend_value;
    }
    else {
        ID_EX_reg->data2 = Regs[instruction->rt];
    }

    //pipeRegister 历厘

    ID_EX_reg->RegDst = signal->RegDst;
    ID_EX_reg->AluSrc = signal->AluSrc;
    ID_EX_reg->MemToReg = signal->MemToReg;
    ID_EX_reg->MemRead = signal->MemRead;
    ID_EX_reg->MemWrite = signal->MemWrite;
    ID_EX_reg->Branch = signal->Branch;
    ID_EX_reg->RegWrite = signal->RegWrite;
    ID_EX_reg->Jump = signal->Jump;
    ID_EX_reg->AluOp[0] = signal->AluOp[0];
    ID_EX_reg->AluOp[1] = signal->AluOp[1];
    ID_EX_reg->extend_value = instruction->extend_value;
    ID_EX_reg->rt = instruction->rt;
    ID_EX_reg->rd = instruction->rd;
    ID_EX_reg->op = instruction->op;
    ID_EX_reg->funct = instruction->funct;
    ID_EX_reg->shamt = instruction->shamt;
    ID_EX_reg->rs = instruction->rs;

    if (signal->Jump == 1) {

        Regs[31] = pc;
        pc = (instruction->j << 2) | (pc & 0xf0000000);
        return;
    }
    //EX harzard

    if (EX_MEM_reg->RegWrite == 1 && signal->Branch != 1 && MEM_WB_reg->RegWrite != 1) {

        if (EX_MEM_reg->rd == ID_EX_reg->rs && ID_EX_reg->rs != 0) {
            ID_EX_reg->data1 = EX_MEM_reg->AluResult;
        }
        else if (EX_MEM_reg->rt == ID_EX_reg->rs && ID_EX_reg->rs != 0 && EX_MEM_reg->type == 'I') {

            ID_EX_reg->data1 = EX_MEM_reg->AluResult;
        }
        else if (EX_MEM_reg->rd == ID_EX_reg->rt && ID_EX_reg->rt != 0) {
            if (ID_EX_reg->type != 'I') {
                ID_EX_reg->data2 = EX_MEM_reg->AluResult;
            }
        }
        else if (EX_MEM_reg->rt == ID_EX_reg->rt && ID_EX_reg->rt != 0 && EX_MEM_reg->type == 'I') {
            if (ID_EX_reg->type != 'I') {
                ID_EX_reg->data2 = EX_MEM_reg->AluResult;
            }
        }
    }
    //MEM harzard

    else if (MEM_WB_reg->RegWrite == 1 && signal->Branch != 1 && ID_EX_reg->op != 35) {
        if (MEM_WB_reg->rd == ID_EX_reg->rs && ID_EX_reg->rs != 0) {
            if (EX_MEM_reg->rd != ID_EX_reg->rs) {
                if (MEM_WB_reg->op == 35) {
                    ID_EX_reg->data1 = MEM_WB_reg->memData;
                }
                else {
                    ID_EX_reg->data1 = MEM_WB_reg->AluResult;
                }
            }
        }
        else if (MEM_WB_reg->rd == ID_EX_reg->rt && ID_EX_reg->rt != 0) {
            if (EX_MEM_reg->rd != ID_EX_reg->rt && ID_EX_reg->type != 'I') {
                if (MEM_WB_reg->op == 35) {
                    ID_EX_reg->data2 = MEM_WB_reg->memData;
                }
                else {
                    ID_EX_reg->data2 = MEM_WB_reg->AluResult;
                }
            }
        }
        else if (MEM_WB_reg->rt == ID_EX_reg->rt && ID_EX_reg->rt != 0) {
            if (MEM_WB_reg->rt == ID_EX_reg->rs && ID_EX_reg->rs != 0) {
                if (EX_MEM_reg->rt != ID_EX_reg->rs) {
                    if (MEM_WB_reg->op == 35) {
                        ID_EX_reg->data1 = MEM_WB_reg->memData;
                    }
                    else {
                        ID_EX_reg->data1 = MEM_WB_reg->AluResult;
                    }
                }
            }
            if (EX_MEM_reg->rt != ID_EX_reg->rt && ID_EX_reg->type != 'I') {
                if (MEM_WB_reg->op == 35) {
                    ID_EX_reg->data2 = MEM_WB_reg->memData;
                }
                else {
                    ID_EX_reg->data2 = MEM_WB_reg->AluResult;
                }
            }
        }
        else if (MEM_WB_reg->rt == ID_EX_reg->rs && ID_EX_reg->rs != 0) {

            if (EX_MEM_reg->rt != ID_EX_reg->rs) {
                if (MEM_WB_reg->op == 35) {
                    ID_EX_reg->data1 = MEM_WB_reg->memData;
                }
                else {
                    ID_EX_reg->data1 = MEM_WB_reg->AluResult;
                }
            }
        }
    }
    //branch stall , forwarding
    else if (signal->Branch == 1) {
        //branchforwarding ex
        if (EX_MEM_reg->RegWrite == 1 && MEM_WB_reg->RegWrite == 1) {
            if (EX_MEM_reg->rt == ID_EX_reg->rt && ID_EX_reg->rt != 0 && MEM_WB_reg->rt == ID_EX_reg->rs) {
                ID_EX_reg->data2 = EX_MEM_reg->AluResult;
                ID_EX_reg->data1 = MEM_WB_reg->memData;
            }
        }
        else if (EX_MEM_reg->RegWrite == 1 && MEM_WB_reg->RegWrite != 1) {

            if (EX_MEM_reg->rd == ID_EX_reg->rs && ID_EX_reg->rs != 0) {
                if (ID_EX_reg->op != 43 && ID_EX_reg->op != 35) {
                    ID_EX_reg->data1 = EX_MEM_reg->AluResult;
                }
            }
            else if (EX_MEM_reg->rd == ID_EX_reg->rt && ID_EX_reg->rt != 0) {
                if (ID_EX_reg->type != 'I') {
                    ID_EX_reg->data2 = EX_MEM_reg->AluResult;
                }
            }
            else if (EX_MEM_reg->rt == ID_EX_reg->rs && ID_EX_reg->rs != 0 && EX_MEM_reg->type == 'I') {
                if (ID_EX_reg->op != 43 && ID_EX_reg->op != 35) {
                    ID_EX_reg->data1 = EX_MEM_reg->AluResult;
                }
            }
            else if (EX_MEM_reg->rt == ID_EX_reg->rt && ID_EX_reg->rt != 0 && EX_MEM_reg->type == 'I') {
                if (ID_EX_reg->type != 'I') {
                    ID_EX_reg->data2 = EX_MEM_reg->AluResult;
                }
            }
        }
        else if (MEM_WB_reg->RegWrite == 1 && EX_MEM_reg->RegWrite != 1) {

            if (MEM_WB_reg->rd == ID_EX_reg->rs && ID_EX_reg->rs != 0) {

                if (EX_MEM_reg->rd != ID_EX_reg->rs) {
                    if (MEM_WB_reg->op == 35) {
                        ID_EX_reg->data1 = MEM_WB_reg->memData;
                    }
                    else {
                        ID_EX_reg->data1 = MEM_WB_reg->AluResult;
                    }
                }
            }
            else if (MEM_WB_reg->rd == ID_EX_reg->rt && ID_EX_reg->rt != 0) {

                if (EX_MEM_reg->rd != ID_EX_reg->rt) {
                    if (MEM_WB_reg->op == 35) {
                        ID_EX_reg->data2 = MEM_WB_reg->memData;
                    }
                    else {
                        ID_EX_reg->data2 = MEM_WB_reg->AluResult;
                    }
                }
            }
            else if (MEM_WB_reg->rt == ID_EX_reg->rt && ID_EX_reg->rt != 0) {

                if (EX_MEM_reg->rt != ID_EX_reg->rt) {
                    if (MEM_WB_reg->op == 35) {
                        ID_EX_reg->data2 = MEM_WB_reg->memData;
                    }
                    else {
                        ID_EX_reg->data2 = MEM_WB_reg->AluResult;
                    }
                }
            }
            else if (MEM_WB_reg->rt == ID_EX_reg->rs && ID_EX_reg->rs != 0) {
                if (EX_MEM_reg->rt != ID_EX_reg->rs) {
                    if (MEM_WB_reg->op == 35) {
                        ID_EX_reg->data1 = MEM_WB_reg->memData;
                    }
                    else {
                        ID_EX_reg->data1 = MEM_WB_reg->AluResult;
                    }
                }
            }
        }
        if (EX_MEM_reg->op == 35) {
            IF_stall = 1;
            return;
        }

        unsigned AluResult = ID_EX_reg->data1 - ID_EX_reg->data2;
        if (signal->Branch == 1 && AluResult == 0 && instruction->op == 4) {
            pc = pc + (instruction->extend_value << 2);
            return;
        }
        else if (signal->Branch == 1 && AluResult != 0 && instruction->op == 5) {
            pc = pc + (instruction->extend_value << 2);
            return;
        }
    }

    //jr stall
    if (ID_EX_reg->op == 0 && ID_EX_reg->funct == 8) {
        ID_stall = 1;
    }

}
void executeStage(EX_MEM_Reg* EX_MEM_reg, ID_EX_Reg* ID_EX_reg, IF_ID_Reg* IF_ID_reg) {

    if (pc == 0xFFFFFFFF) {
        return;
    }
    //sw forwading
    unsigned AluResult = 0;
    if (ID_EX_reg->Branch != 1 && ID_EX_reg->Jump != 1) {
        switch (ID_EX_reg->op) {
        case 0: //r-type
            switch (ID_EX_reg->funct) {
            case 32: //add
                AluResult = ID_EX_reg->data1 + ID_EX_reg->data2;
                break;
            case 33: //addu
                AluResult = ID_EX_reg->data1 + ID_EX_reg->data2;
                break;
            case 34: //sub
                AluResult = ID_EX_reg->data1 - ID_EX_reg->data2;
                break;
            case 35: //subu
                AluResult = ID_EX_reg->data1 - ID_EX_reg->data2;
                break;
            case 36: //and
                AluResult = ID_EX_reg->data1 & ID_EX_reg->data2;
                break;
            case 37: //or //move
                if (ID_EX_reg->rt == 0) {
                    AluResult = ID_EX_reg->data1 + 0;
                }
                else {
                    AluResult = ID_EX_reg->data1 | ID_EX_reg->data2;
                }
                break;
            case 42: //slt
                AluResult = (ID_EX_reg->data1 < ID_EX_reg->data2) ? 1 : 0;
                break;
            case 0: //sll
                AluResult = ID_EX_reg->data2 << ID_EX_reg->shamt;
                break;
            case 2: //srl
                AluResult = ID_EX_reg->data2 >> ID_EX_reg->shamt;
                break;
            case 24: //mul
                AluResult = ID_EX_reg->data1 * ID_EX_reg->data2;
                break;
            case 26: //div
                AluResult = ID_EX_reg->data1 / ID_EX_reg->data2;
                break;
            case 8: //jr
                AluResult = ID_EX_reg->data1;
                break;
            }
        case 2: //j
            break;
        case 3: //jal
            break;
        case 4: //beq
            AluResult = ID_EX_reg->data1 - ID_EX_reg->data2;
            break;
        case 5: //bne
            AluResult = ID_EX_reg->data1 - ID_EX_reg->data2;
            break;
        case 8: //addi
            AluResult = ID_EX_reg->data1 + ID_EX_reg->data2;
            break;
        case 9: //addiu
            AluResult = ID_EX_reg->data1 + ID_EX_reg->data2;
            break;
        case 10: //slti
            AluResult = (ID_EX_reg->data1 < ID_EX_reg->data2) ? 1 : 0;
            break;
        case 35: //load
            AluResult = ID_EX_reg->data2 + ID_EX_reg->data1;
            break;
        case 43: //store
            AluResult = ID_EX_reg->data2 + ID_EX_reg->data1;
            break;
        }
    }

    //pipelineRegister 历厘
    EX_MEM_reg->MemToReg = ID_EX_reg->MemToReg;
    EX_MEM_reg->MemWrite = ID_EX_reg->MemWrite;
    EX_MEM_reg->MemRead = ID_EX_reg->MemRead;
    EX_MEM_reg->RegWrite = ID_EX_reg->RegWrite;
    EX_MEM_reg->AluResult = AluResult;
    EX_MEM_reg->rt = ID_EX_reg->rt;
    EX_MEM_reg->rd = ID_EX_reg->rd;
    EX_MEM_reg->instruction = ID_EX_reg->instruction;
    EX_MEM_reg->op = ID_EX_reg->op;
    EX_MEM_reg->funct = ID_EX_reg->funct;
    EX_MEM_reg->rs = ID_EX_reg->rs;
    EX_MEM_reg->extend_value = ID_EX_reg->extend_value;
    EX_MEM_reg->type = ID_EX_reg->type;
    EX_MEM_reg->Branch = ID_EX_reg->Branch;

    if (EX_MEM_reg->op == 0 && EX_MEM_reg->funct == 8) {
        EX_stall = 1;
    }

}
void memoryStage(EX_MEM_Reg* EX_MEM_reg, MEM_WB_Reg* MEM_WB_reg, ID_EX_Reg* ID_EX_reg) {


    if (pc == 0xFFFFFFFF) {
        return;
    }
    unsigned mem_data = 0;
    if (EX_MEM_reg->MemWrite == 1 && EX_MEM_reg->MemRead == 0 && EX_MEM_reg->Jump != 1 && EX_MEM_reg->Branch != 1) //sw 
    {
        DataMem[EX_MEM_reg->AluResult / sizeof(unsigned int)] = Regs[EX_MEM_reg->rt];
        MemoryCount++;
    }
    else if (EX_MEM_reg->MemWrite == 0 && EX_MEM_reg->MemRead == 1) //lw 
    {
        if (EX_MEM_reg->rt == MEM_WB_reg->rt && MEM_WB_reg->op == 43) {

            MEM_WB_reg->memData = Regs[EX_MEM_reg->rt];
        }
        MEM_WB_reg->memData = DataMem[EX_MEM_reg->AluResult / sizeof(unsigned int)];
        MemoryCount++;
    }
    MEM_WB_reg->instruction = EX_MEM_reg->instruction;
    MEM_WB_reg->op = EX_MEM_reg->op;
    MEM_WB_reg->funct = EX_MEM_reg->funct;
    MEM_WB_reg->AluResult = EX_MEM_reg->AluResult;
    MEM_WB_reg->rt = EX_MEM_reg->rt;
    MEM_WB_reg->rd = EX_MEM_reg->rd;
    MEM_WB_reg->RegWrite = EX_MEM_reg->RegWrite;
    MEM_WB_reg->rs = EX_MEM_reg->rs;
    MEM_WB_reg->extend_value = EX_MEM_reg->extend_value;
    MEM_WB_reg->type = EX_MEM_reg->type;
    MEM_WB_reg->Branch = EX_MEM_reg->Branch;
}
void writebackStage(MEM_WB_Reg* MEM_WB_reg, WBPrint* WB_print_data) {


    if (pc == 0xFFFFFFFF) {
        return;
    }
    if (MEM_WB_reg->AluResult == -1 && MEM_WB_reg->op == 0 && MEM_WB_reg->funct == 8) {
        pc = MEM_WB_reg->AluResult;
        return;
    } //jr control

    else if (MEM_WB_reg->RegWrite == 1 && MEM_WB_reg->Jump != 1 && MEM_WB_reg->Branch != 1) {

        switch (MEM_WB_reg->op) {
        case 0:
            if (MEM_WB_reg->funct != 8) {
                Regs[MEM_WB_reg->rd] = MEM_WB_reg->AluResult;
            } //jr力寇
            break;
        case 8:
            Regs[MEM_WB_reg->rt] = MEM_WB_reg->AluResult;
            break;
        case 9:
            if (MEM_WB_reg->rs == 0) {
                Regs[MEM_WB_reg->rt] = MEM_WB_reg->extend_value;
            } //ii
            else {
                Regs[MEM_WB_reg->rt] = MEM_WB_reg->AluResult;
            }
            break;
        case 10:
            Regs[MEM_WB_reg->rt] = MEM_WB_reg->AluResult;
            break;
        case 35:
            Regs[MEM_WB_reg->rt] = MEM_WB_reg->memData;
            break;
        }
    }
    WB_print_data->branch = MEM_WB_reg->Branch;
    WB_print_data->Jump = MEM_WB_reg->Jump;
    WB_print_data->funct = MEM_WB_reg->funct;
    WB_print_data->AluResult = MEM_WB_reg->AluResult;
    WB_print_data->memData = MEM_WB_reg->memData;
    WB_print_data->extend_value = MEM_WB_reg->extend_value;
    WB_print_data->rd = MEM_WB_reg->rd;
    WB_print_data->rt = MEM_WB_reg->rt;
    WB_print_data->rs = MEM_WB_reg->rs;
    WB_print_data->op = MEM_WB_reg->op;
    WB_print_data->RegWrite = MEM_WB_reg->RegWrite;

}
void printResult(IF_ID_Reg* IF_ID_reg, ID_EX_Reg* ID_EX_reg, EX_MEM_Reg* EX_MEM_reg, MEM_WB_Reg* MEM_WB_reg, WBPrint* WB_print_data)
{
    printf("Cycle[%d]\n", clockCycle);
    printf("[IF]\n");
    if (IF_stall != 1) {
        printf("    [IM] Instruction-opcode:%d\n", IF_ID_reg->op);
        printf("    [PC Update] pc:%x\n", pc);
    }
    else {
        printf("    Bubble\n");
    }
    printf("[ID]\n");
    if (ID_EX_reg->type == 'R' && ID_stall != 1) {
        printf("    type:%c, opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d),  rd:%d, shamt:%d, funct:%d\n", ID_EX_reg->type, ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->rd, ID_EX_reg->shamt, ID_EX_reg->funct);
    }
    else if (ID_EX_reg->type == 'I' && ID_stall != 1) {
        switch (ID_EX_reg->op) {
        case 4:
            printf("    type:Branch(I type), opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d). imm:%d\n", ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->extend_value);
            break;
        case 5:
            printf("    type:Branch(I type), opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d). imm:%d\n", ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->extend_value);
            break;
        case 35:
            printf("    type:LW(I type), opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d). imm:%d\n", ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->extend_value);
            break;
        case 43:
            printf("    type:SW(I type), opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d). imm:%d\n", ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->extend_value);
            break;
        default:
            printf("    type:%c, opcode:%d, rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d). imm:%d\n", ID_EX_reg->type, ID_EX_reg->op, ID_EX_reg->rs, ID_EX_reg->rs, Regs[ID_EX_reg->rs], ID_EX_reg->rt, ID_EX_reg->rt, Regs[ID_EX_reg->rt], ID_EX_reg->extend_value);
        }
    }
    else if (ID_EX_reg->type == 'J') {
        printf("    type:%c, opcode:%d\n", ID_EX_reg->type, ID_EX_reg->op);
    }
    else if (ID_EX_reg->type = 'X' || ID_stall == 1) {
        printf("    Bubble\n");
    }
    printf("[EX]\n");
    if (EX_MEM_reg->type == 'R' && EX_stall != 1) {
        printf("    rs:%d(R[%d]=%d),  rt:%d(R[%d]=%d), aluResult:%d\n", EX_MEM_reg->rs, EX_MEM_reg->rs, Regs[EX_MEM_reg->rs], EX_MEM_reg->rt, EX_MEM_reg->rt, Regs[EX_MEM_reg->rt], EX_MEM_reg->AluResult);
    }
    else if (EX_MEM_reg->type == 'I' && EX_stall != 1) {
        switch (EX_MEM_reg->op) {
        case 4:
            printf("    Branch Bubble\n");
            break;
        case 5:
            printf("    Branch Bubble\n");
            break;
        case 35:
            printf("    rs:%d(R[%d]=%d). imm:%d, address:%d\n", EX_MEM_reg->rs, EX_MEM_reg->rs, Regs[EX_MEM_reg->rs], EX_MEM_reg->extend_value, EX_MEM_reg->AluResult);
            break;
        case 43:
            printf("    rs:%d(R[%d]=%d). imm:%d, address:%d\n", EX_MEM_reg->rs, EX_MEM_reg->rs, Regs[EX_MEM_reg->rs], EX_MEM_reg->extend_value, EX_MEM_reg->AluResult);
            break;
        default:
            printf("    rs:%d(R[%d]=%d). imm:%d, aluResult:%d\n", EX_MEM_reg->rs, EX_MEM_reg->rs, Regs[EX_MEM_reg->rs], EX_MEM_reg->extend_value, EX_MEM_reg->AluResult);
        }
    }
    else if (EX_MEM_reg->type == 'J') {
        printf("   Jump\n");
    }
    else if (EX_MEM_reg->type = 'X' || EX_stall == 1 && EX_MEM_reg->Branch != 1) {
        printf("    Bubble\n");
    }
    printf("[MEM]\n");
    if (MEM_WB_reg->op == 35) {
        printf("    LW Case : R[%d] = %d\n", MEM_WB_reg->rt, MEM_WB_reg->memData);
    }
    else if (MEM_WB_reg->op == 43) {
        printf("    SW Case : DataMem = %d\n", Regs[EX_MEM_reg->rt]);
    }
    else if (MEM_WB_reg->op == 4 || MEM_WB_reg->op == 5) {
        printf("    Branch Bubble\n");
    }
    else if (MEM_WB_reg->type == 'J') {
        printf("    Jump\n");
    }
    else if (MEM_WB_reg->type = 'X' || MEM_stall == 1) {
        printf("    Bubble\n");
    }
    printf("[WB]\n");
    if (WB_print_data->RegWrite == 1) {

        switch (WB_print_data->op) {
        case 0:
            if (WB_print_data->funct != 8) {
                printf("    R[%d]: %d\n", WB_print_data->rd, WB_print_data->AluResult);
            } //jr力寇
            break;
        case 8:
            printf("    R[%d]: %d\n", WB_print_data->rt, WB_print_data->AluResult);
            break;
        case 9:
            if (WB_print_data->rs == 0) {
                printf("    R[%d]: %d\n", WB_print_data->rt, WB_print_data->extend_value);
            } //ii
            else {
                printf("    R[%d]: %d\n", WB_print_data->rt, WB_print_data->AluResult);
            }
            break;
        case 10:
            printf("    R[%d]: %d\n", WB_print_data->rt, WB_print_data->AluResult);
            break;
        case 35:
            printf("    R[%d]: %d\n", WB_print_data->rt, WB_print_data->memData);
            break;
        }
    }
    else if (WB_print_data->branch == 1) {
        printf("    Branch Bubble\n");
    }
    else if (WB_print_data->Jump == 1) {
        printf("    Jump\n");
    }
    else {
        printf("    Bubble\n");
    }
    printf("\n\n");
    clockCycle++;
}