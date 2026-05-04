#include <stdio.h>

unsigned int mbr; 
unsigned short mar, pc, imm; 
unsigned char ir, ro0, ro1, e, l, g; 
unsigned short reg[8]; 
unsigned char memoria[256];


void fetch();
void decode();

int main(){


    return 0;
}


void fetch(){
    mar = pc;
    mbr = memoria[mar];
    mar++;
    mbr = (mbr << 8) + memoria[mar];
    mar++;
    mbr = (mbr << 8) + memoria[mar];

}

void decode(){
    ir = mbr >> 19;
    if(ir >= 0b00000 && ir <= 0b00001){
        return
    }

    if(ir >= 0b01110 && ir <= 0b10100){
        mbr = mbr << 8;
        pc = mbr >> 7;
    }

    if(ir >= 0b10101 && ir <= 0b10110){
        mbr = mbr << 8;
        mar = mbr >> 7;
    }

    if(ir >= 0b10111 && ir <= 0b11101){
        ro0 = (mbr << 13) >> 29;
        mbr = mbr << 8;
        imm = mbr >> 7;
    }


}


