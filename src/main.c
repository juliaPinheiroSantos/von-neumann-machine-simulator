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
    
    // instrução com 8 bits - hlt (0) e nop (1)
    // formato: [0000 0000] [opcode(5bits) 000] [8bits outra instrução] [8bits outra instrução]
    // ambas não fazem "nada" no decode(), então só pegamos o opcode delas e guardamos em ir
    

    if(ir >= 0 && ir <= 1){
        return
    }

    /** instrução com 16 bits - ldr (2) até xor (12)
    formato: [0000 0000] [opcode(5bits) reg0(3bits)] [reg1(3bits) 0 0000] [8bits outra instrução] 
    todas guardam os índices dos registradores em reg0 (1º registrador da instrução) e em reg1 (2º registrador da instrução)
    para pegar reg0 (ou ro0), precisamos deslocar 13bits para a esquerda e depois 29bits para a direita
    para pegar reg1 (ou ro1), precisamos deslocar 16bits para a esquerda e depois 29bits para a direita
    */
    if(ir >= 2 && ir <= 12){ 
        ro0 = (mbr << 13) >> 29;
        ro1 = (mbr << 16) >> 29;
    }

    /** instrução com 8 bits - not (13)
    formato: [0000 0000] [opcode(5bits) reg0(3bits)] [8bits outra instrução] [8bits outra instrução]
    o índice do registrador da instrução precisa ficar no reg0 (ou ro0)
    para pegar o reg0, precisamos deslocar 13bits para a esquerda e depois 29bits para a direita
     */
    if(ir == 13){
        ro0 = (mbr << 13) >> 29;
    }

    /** instrução com 24 bits - je (14) até jmp (20) 
    formato: [0000 0000] [opcode(5bits) 000] [endereço de memória(8bits)] [endereço de memória(8bits)]
    o endereço de memória precisa ficar no registrador PC
    para pegar o endereço de memória, precisamos deslocar 16 bits para a esquerda e depois 16 bits para a direita
    */
    if(ir >= 14 && ir <= 20){
        pc = (mbr << 16) >> 16;
    }

    /** instrução com 24 bits - ld (21) até st (22)
    formato: [0000 0000] [opcode(5bits) reg0(3bits)] [endereço de memória(8bits)] [endereço de memória(8bits)] 
    o índice do registrador da instrução precisa ficar no reg0 (ou ro0)
        para pegar o reg0, precisamos deslocar 13 bits para a esquerda e depois 29 bits para a direita
    o endereço de memória precisa ficar no registrador mar
        para pegar o endereço de memória, precisamos deslocar 16bits para a esquerda e depois 16bits para a direita
    */
    if(ir >= 21 && ir <= 22){
        ro0 = (mbr << 13) >> 29;
        mar = (mbr << 16) >> 16;

    }


    /** instrução com 24 bits - movi(23) até rsh (29)
    formato: [0000 0000] [opcode(5bits) reg0(3bits)] [endereço de memória(8bits)] [endereço de memória(8bits)] 
    o índice do registrador da instrução precisa ficar no reg0 (ou ro0)
        para pegar o reg0, precisamos deslocar 13 bits para a esquerda e depois 29 bits para a direita
    o imediato precisa ficar no registrador imm
        para pegar o imediato, precisamos deslocar 16bits para a esquerda e depois 16bits para a direita
    */
    if(ir >= 23 && ir <= 29){
        ro0 = (mbr << 13) >> 29;
        imm = (mbr << 16) >> 16;
    }


}



void execute(){
    switch(ir){
        case 1: pc++;
        case 2: 
    }



}


