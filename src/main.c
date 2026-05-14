#include <stdio.h>
#include <stdint.h>

/* DADOS IMPORTANTES:
    1. memória é um vetor de 256 posições com tam de 8 bits;
    2. memória usa um barramento com oito linhas (bits) de dados 
        todas as transferências entre MBR e a memória devem ser de byte em byte;
    3. necessário ler um arquivo texto para carregar a memória com instruções e dados;
    4. a CPU processa apenas números inteiros contidos em palavras de 16 bits;r
*/ 

unsigned int mbr;                       // memory buffer register: dado lido ou a ser escrito na memória (32 bits) 
unsigned short int mar,                 // memory address register: endereço de memória a ser acessado (16 bits)
                   pc,                  // program counter: endereço do próximo par de instrução
                   imm;                 // immediate: valor constante
unsigned char ir,                       // instruction register: opcode da instrução atual (5 bits)
              ro0,                      // register operand 0: indica o endereço do primeiro operando registrador do reg[8] a instrução quer acessar
              ro1,                      // register operand 1: indica o endereço do segundo operando registrador do reg[8] a instrução quer acessar                
              e,                        // flag Equal (cmp): 1 se rX == rY
              l,                        // flag Lower (cmp): 1 se rX < rY
              g;                        // flag Greater (cmp): 1 se rX > rY
unsigned short reg[8]; 
unsigned char memoria[256];             // a memória será um vetor de 256 posições com tamanho de 8 bits

void fetch();                           // declaração da função fetch() "busca"
void decode();                          // declaração da função decode() "decodificação"
int execute();                          // declaração da função execute() "execução"

int main(){
    int flag = 0;
    do{
        fetch();
        decode();
        flag = execute();
    }while(flag == 1);

    return 0;
}

/**
 * Função de busca, responsável por buscar o próximo endereço de memória e armazenar no registrador mbr
    Assumimos que cada instrução poderá ter 1, 2 ou 4 (bytes). Cada endereço de memória armazena 1 byte (8 bits)
    Vamos buscar sempre 3 bytes e inserir no mbr através de operações bit-a-bit de "shift"
 */
void fetch(){
    mar = pc;                           // o registrador mar recebe o endereço da próxima instrução armazenada no registrador pc
                                        // memoria[mar]: [byte 1]     
    
    mbr = memoria[mar];                 // o registrador mbr recebe os primeiros 8 bits da palavra de instrução
                                        // mbr = [0000 0000] [0000 0000] [0000 0000] [byte 1]
     
    mar++;                              // incrementa o endereço para pegar os próximos 8 bits da instrução
                                        // memoria[mar]: [byte 2]
    
    mbr = (mbr << 8) + memoria[mar];    // o registrador mbr recebe os próximos 8 bits da palavra de instrução
                                        // mbr = [0000 0000] [0000 0000] [byte 1] [byte 2]
   
    mar++;                              // incrementa o endereço para pegar os próximos 8 bits da instrução
                                        // memoria[mar]: [byte 3]
                                        
    mbr = (mbr << 8) + memoria[mar];    // o registrador mbr recebe os próximos 8 bits da palavra de instrução
                                        // mbr = [0000 0000] [byte 1] [byte 2] [byte 3]

}


/** Função de decodificação, responsável por "fatiar" a palavra de memória buscada e distribuir cada "fatia" para um registrador designado.
    O estado atual do registrador mbr é: [0000 0000] [byte 1] [byte 2] [byte 3].
    As instruções podem ser de 5 tipos diferentes:
        [opcode | 0]
        [opcode | reg0 | reg 1 | 0]
        [opcode | reg0 ]
        [opcode | 0    | endereço de memória]
        [opcode | reg0 | endereço de memória ou valor imediato]

    Ou seja, quando buscamos sempre 3 bytes na função fetch(), pode sim acontecer de, ao buscar a instrução nop que tem esse formato: [opcode | 0],
    tenhamos no mbr as seguintes informações [0000 0000] [0000 1000] [0001 0000] [0000 0000]
        onde:
            000010 é opcode de nop, os 000 seguintes faz parte dessa instrução
            00100 é o opcode de add, os 000 seguintes indicam o índice do 1º registrador dessa instrução, e os 000 seguintes a estes indicam o índice do 2º registrador dessa instrução.
        Usar o PC (program counter) resolve isso para nós: a cada ciclo de busca-decodifica-executa, o pc é incrementado para a próxima posição de memória que possui uma instrução.
        Então, se nop esta no endereço 10, o PC conterá 10 até o final desse ciclo de instrução. 
        No próximo ciclo, ao ser incrementado, ele passa a apontar para a posição 11, que indica a instrução add seguinte.
        Mesmo que no mbr quando pc = 10 tinha parte da próxima instrução, essa parte é "descartada" e "lida novamente" no próximo ciclo.
    
 */
void decode(){
    
    ir = mbr >> 19;                         // o mbr contém a palavra de instrução inteira [0000 0000] [0101 1111] [1111 1111] [0000 0000]
                                            // como queremos somente os bits de 23:19 referentes ao opcode, deslocamos para a direita em 19 bits.
                                            // [0000 0000] [0101 1111] [1111 1111] [0000 0000] -> [0000 0000] [0000 0000] [0000 0000] [0000 1011]
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



void execute(){

}


