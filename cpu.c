#include <stdio.h>

unsigned int mbr;         // memory buffer register: dado lido ou a ser escrito na memória
unsigned short int  mar,  // memory address register: endereço de memória a ser acessado
                    pc,   // program counter: endereço do próximo par de instrução
                    imm;  // immediate: valor constante
unsigned char   ir,       // instruction register: opcode da instrução atual
                ro0, ro1; // índice de quais registradores dentro do reg[8] usar
unsigned char   e,        // flag Equal (cmp): 1 se rX == rY 
                l,        // flag Lower (cmp): 1 se rX < rY 
                g;        // flag Greater (cmp): 1 se rX > rY 
unsigned short int reg[8];       
unsigned char memoria[256];      

void busca(){

}

void decodifica(){

}

void executa(int){

}

int main(){
	int flag = 0;
	do{
		busca();
		decodifica();
		flag = executa(); // quando for halt
        // impressão
	} while(flag == 1);
}