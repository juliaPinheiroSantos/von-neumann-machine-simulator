#include <stdio.h>

/* DADOS IMPORTANTES:
1. memória é um vetor de 256 posições com tam de 8 bits;
2. memória usa um barramento com oito linhas (bits) de dados 
    todas as transferências entre MBR e a memória devem ser de byte em byte;
3. necessário ler um arquivo texto para carregar a memória com instruções e dados;
4. a CPU processa apenas números inteiros contidos em palavras de 16 bits;
*/ 

unsigned int mbr;         // memory buffer register: dado lido ou a ser escrito na memória (32 bits)
unsigned short int  mar,  // memory address register: endereço de memória a ser acessado (16 bits)
                    pc,   // program counter: endereço do próximo par de instrução
                    imm;  // immediate: valor constante
unsigned char   ir,       // instruction register: opcode da instrução atual (8 bits)
                ro0, ro1, // índice de quais registradores dentro do reg[8] usar
                e,        // flag Equal (cmp): 1 se rX == rY 
                l,        // flag Lower (cmp): 1 se rX < rY 
                g,        // flag Greater (cmp): 1 se rX > rY 
                memoria[256];
unsigned short int reg[8];       

void busca(){
    // objetivo da busca: CPU buscar uma palavra da memória
    mar = pc; // pc aponta p/ a próxima instrução a ser buscada, e o mar recebe esse valor (questões de barramento)
    mbr = memoria[mar]; // mbr recebe a palavra buscada na memória na posição indicado pelo mar
    mar++; // acessa + 1 posição da memória
    mbr = (mbr << 8) + memoria[mar]; // deslocamento tamanho de 1 palavra
    mar++; 
    mbr = (mbr << 8) + memoria[mar]; 
    // a instrução maior possível tem 3 bytes, então a busca sempre pega 3 posições 
    // de memória pra garantir que pegou tudo, por isso 'mar++' 2 vezes deslocando bits
    // antes de passar para o mbr
}

void decodifica(){
    // considera 32 bits completos do mbr 
    ir = mbr >> 19; 
    // estado atual
    // posição dos bits ->   31....24  23....16   15....8   7.....0
    // mbr ->               [00000000] [byte  1] [byte  2] [byte  3]
    // opcode são os 5 bits mais à esquerda do byte 1
    // ir irá ler os 5 bits menos significativos, ai deslocamentos 19 à direita
    
    // formato de inst: [ opcode | 0 ] (8 bits)
    // hlt & nop -> fazem nada

    // formato de inst: [opcode | reg0 | reg1 | 0 ] (16 bits)
    if (ir >= 2 && ir <= 12){
        ro0 = (mbr << 13) >> 29; 
        // [rrr00000] [00000000] [00000000] [00000000] (<< 13) -> removemos campo opcode
        // [00000000] [00000000] [00000000] [00000rrr] (>> 29), 32 - 29 = 3 (tam reg)
        ro1 = (mbr << 16) >> 29; // tira o campo opcode + ro0 depois desloca 29 bits à direita
    } else if(ir == 13){
        // not (8 bits)
        ro0 = (mbr << 13) >> 29;
    }
}

void executa(int){

}

int main(){
	int flag = 0;
	do{
		busca();
		decodifica();
		// flag = executa(); quando for hlt
        // impressão
	} while(flag == 1);
}