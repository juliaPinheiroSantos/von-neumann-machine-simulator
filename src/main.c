#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "loader.h"
#define BG_YELLOW "\033[30;103m"        // define o código de cor ANSI para pintar o fundo de amarelo na impressão dos dados da CPU, igual está no trabalho
#define RESET "\033[0m"                 // reseta para a formação normal do terminal quando não for impressão da CPU

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
unsigned char memoria[256] = {0};       // a memória será um vetor de 256 posições com tamanho de 8 bits

void fetch();                           // declaração da função fetch() "busca"
void decode();                          // declaração da função decode() "decodificação"
int execute();                          // declaração da função execute() "execução"
void imprimirEstado();

int main(){
    int flag = 0;
    imprimirEstado();
    carregar_memoria("programa.txt", memoria);
    imprimirEstado();
    do{
        fetch();
        decode();
        flag = execute();
    }while(flag == 0);

    return 0;
}

/**
 * Função de busca, responsável por buscar o próximo endereço de memória e armazenar no registrador mbr
    Assumimos que cada instrução poderá ter 1, 2 ou 3 (bytes). Cada endereço de memória armazena 1 byte (8 bits)
    Vamos buscar sempre 3 bytes e inserir no mbr através de operações bit-a-bit de "shift"
 */
void fetch(){
    mar = pc;                           // o registrador mar recebe o endereço da próxima instrução armazenada no registrador pc
                                        // memoria[mar]: [byte 1]     
    
    mbr = memoria[mar];                 // o registrador mbr recebe os primeiros 8 bits da palavra de instrução
                                        // mbr = [0000 0000] [0000 0000] [0000 0000] [byte 1]
    
    
    mar++;                              // incrementa o endereço para pegar os próximos 8 bits da instrução
                                        // memoria[mar]: [byte 2]
    
    mbr = (mbr << 8) | memoria[mar];    // o registrador mbr recebe os próximos 8 bits da palavra de instrução
                                        // mbr = [0000 0000] [0000 0000] [byte 1] [byte 2]
   
    mar++;                              // incrementa o endereço para pegar os próximos 8 bits da instrução
                                        // memoria[mar]: [byte 3]
                                        
    mbr = (mbr << 8) | memoria[mar];    // o registrador mbr recebe os próximos 8 bits da palavra de instrução
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
        return;
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
        mar = (mbr << 16) >> 16;
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


/** Função de busca, responsável por executar a instrução de acordo com o seu opcode (armazenado no registrador ir).
    O estado atual de cada registrador depende: da instrução buscada anteriormente e da instrução buscada no atual ciclo de máquina.
    O incremento do pc depois da execução de cada instrução depende do tamanho da instrução que foi executada:
        se a instrução tem 1 byte, incrementa o pc em 1
        se a instrução tem 2 bytes, incrementa o pc em 2
        se a instrução tem 3 bytes, incrementa o pc em 3
    Todo endereço de memória deve ser indicado pelo mar.
    Todo o tráfego de e para a memória RAM deve passar pelo mbr.
 */

int execute(){
    switch(ir){
        case 0: /** hlt -> para o programa
            como o nosso ciclo de máquina continua funcionando while(flag == 0), 
            retornar 1 nesse case vai fazer o ciclo parar.
             */
            return 1;
        case 1: 
            pc++;
            break;

        case 2: /**ldr rX, rY -> rX = *rY
            o que faz: carrega no registrador rX o conteúdo do endereço indicado pelo registrador rY
            se queremos pegar o conteúdo do endereço indicado por rY, precisamos que o endereço desse conteúdo esteja no mar
                além disso, o conteúdo depois de buscado, precisa estar no mbr
            depois disso é que esse conteúdo deve ser inserido no registrador indicado por rX
            */ 
            mar = reg[ro1];                             // o endereço está em rY (que é o índice guardado em ro1)
            
            // considerando que o dado tem 16bits, cada índice de memória guarda 8bits e o barramento também é de 8bits
            // precisamos pegar o conteúdo do endereço indicado por mar (sendo mar = reg[ro1]) em duas partes: 
            mbr = memoria[mar];                         // memoria[mar] = 8bits MSB do dado
            mbr = (mbr << 8) + memoria[mar + 1];        // memoria[mar + 1] = 8 bits LSB do dado
            reg[ro0] = mbr;                             // guardamos o conteúdo de mbr (que é o dado buscado na posição reg[reo1]) dentro do ro0
            pc = pc + 2;                                // como a instrução ldr tem 16 bits, não podemos incrementar o pc só em 1, porque isso iria fazer com que ele apontasse para os 8 bits LSB dessa instrução
                                                        // portanto, incrementamos em 2 para pegar a próxima instrução válida.
            break;
                                                        
        case 3: /**str rX, rY -> ∗rY = rX 
            o que faz: armazena no endereço indicado por rY o valor do registrador rX
            se queremos pegar o endereço de rY (que é reg[ro1]), precisamos que esse endereço esteja no mar
            se queremos armazenar o conteúdo de rX (que é reg[ro0]) no endereço de rY, precisamos que o conteúdo de rX esteja no mbr
            */

            mar = reg[ro1];
            mbr = reg[ro0];
            // estado atual do mbr: [0000 0000] [0000 0000] [8bits MSB do dado] [8bits LSB do dado]
            // a gravação na memória deve ser feita de byte em byte
            // memoria[mar] = 8 bits MSB de mbr
            memoria[mar] = (mbr << 16) >> 24;
            // memoria[mar + 1] = 8 bits LSB de mbr
            memoria[mar + 1] = (mbr << 24) >> 24;
            pc = pc + 2;
            break;

        case 4: /** add rX, rY -> rX = rX + rY
            o que faz: adiciona o conteúdo de rX e rY e guarda esse valor em rX
             */
             reg[ro0] = reg[ro0] + reg[ro1];
             pc = pc + 2;
             break;

        case 5: /** sub rX, rY -> rX = rX - rY
            o que faz: subtrái do conteúdo de rX o valor de rY, e guarda esse valor em rX
            */
            reg[ro0] = reg[ro0] - reg[ro1];
            pc = pc + 2;
            break;
        
        case 6: /** mul rX, rY -> rX = rX * rY
            o que faz: multiplica o conteúdo de rX pelo conteúdo de rY, e guarda esse valor em rX
            */
            reg[ro0] = reg[ro0] * reg[ro1];
            pc = pc + 2;
            break;
        
        case 7: /** div rX, rY -> rX = rX / rY
            o que faz: divide o conteúdo de rX pelo conteúdo de rY, e guarda esse valor em rX
            não precisamos nos preocupar com um resultado de "valor flutuante" no caso dessa divisão de inteiros, pois o C já fica responsável por ignorar o resto e considerar só o inteiro
            entretanto, no caso de divisão por 0, isso pode dar algum erro
             */
            if(reg[ro1] != 0){
                reg[ro0] = reg[ro0] / reg[ro1];
            } else {
                printf("Divisão por zero não é permitida");
            }
             
            pc = pc + 2;
            break;
        
        case 8: /** cmp rX, rY 
            o que faz: compara o conteúdo de rX com o conteúdo de rY
            se rX = rY, então e = 1; senão, e = 0
            se rX < rY, então l = 1; senão, l = 0
            se rX > rY, então g = 1; senão g = 0
         */

            
            if(reg[ro0] == reg[ro1]){
                e = 1;
            } else {
                e = 0;
            }

            if(reg[ro0] < reg[ro1]){
                l = 1;
            } else {
                l = 0;
            }

            if(reg[ro0] > reg[ro1]){
                g = 1;
            } else {
                g = 0;
            }

            pc = pc + 2;

            break;
        
        case 9: /** movr rX, rY -> rX = rY
            o que faz: substitui o valor de rX pelo valor de rY, é uma atribuição simples
            */
            reg[ro0] = reg[ro1];
            pc = pc + 2;
            break;
        
        case 10: /** and rX, rY -> rX = rX & rY 
            o que faz: operação bit-a-bit. realiza a operação lógica '&' entre rX e rY e guarda o resultado dessa operação em rX
            o operaçao '&' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna
            se nessa coluna ambos os bits forem 1, ele resulta em 1. se um deles for 0, resulta em 0
            */

            reg[ro0] = reg[ro0] & reg[ro1];
            pc = pc + 2;
            break;
        
        case 11: /** or rX, rY -> rX = rX | rY
            o que faz: operação bit-a-bit. realiza a operação lógica '|' (or) entre rX e rY e guarda o resultado dessa operação em rX
            a operação '|' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna
            se nessa coluna um dos bits for 1, ele resulta em 1. se ambos os bits forem 0, resulta em 0
             */
            reg[ro0] = reg[ro0] | reg[ro1];
            pc = pc + 2;
            break;


        case 12: /** xor rX, rY -> rX = rX ^ rY
            o que faz: operação bit-a-bit. realiza a operação lógica '^' (xor) entre rX e rY e guarda o resultado dessa operação em rX
            a operação '^' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna
            se nessa coluna os bits forem diferentes entre si, ele resulta em 1. se forem iguais, resulta em 0
             */

            reg[ro0] = reg[ro0] ^ reg[ro1];
            pc = pc + 2;
            break;
        
        case 13: /** not rX ->  rX = !rX 
            o que faz: operação bit-a-bit. ele inverte todos os bits do valor armazenado em rx
            o que é 1 vira 0, e o que é 0 vira 1
            em C, o operador que faz isso é o '~'
            */

            reg[ro0] = ~reg[ro0];
            pc = pc + 1;
            break;
        
        case 14: /** je Z -> PC = Z se E = 1
            o que faz: salto para o endereço Z se a flag E = 1
            o endereço Z vai estar armazendo no registrador mar
            não pode incrementar o pc para a próxima posição porque vai ir para a posição Z que está armazenada em PC agora
            se não entrar na condição, deve buscar a próxima instrução, já que não vai pro endereço Z do salto
             */

             if(e == 1){
                pc = mar;
             } else {
                pc = pc + 3;
             }

             break;
        
        case 15: /** jne Z -> PC = Z se E = 0
             o que faz: salto para o endereço Z se a flag E = 0
             */

             if(e == 0){
                pc = mar;
             } else {
                pc = pc + 3;
             }

             break;
        
        case 16: /** jl Z -> PC = Z se L = 1
             o que faz: salto para o endereço Z se a flag L = 1
             */

             if(l == 1){
                pc = mar;
             } else {
                pc = pc + 3;
             }

             break;

        case 17: /** jle Z -> PC = Z se E = 1 ou L = 1
             o que faz: salto para o endereço Z se a flag E = 1 ou a flag L = 1
             */
            
             if(e == 1 || l == 1){
                pc = mar;
             } else {
                pc = pc + 3;
             }
             break;

        case 18: /** jg Z -> PC = Z se G = 1
            o que faz: salto para o endereço Z se a flag G = 1
             */
            
             if(g == 1){
                pc = mar;
             } else {
                pc = pc + 3;
             }

             break;
        
        case 19: /** jge Z -> PC = Z se E = 1 ou G = 1
             */

             if(e == 1 || g == 1){
                pc = mar;
             } else {
                pc = pc + 3;
             }

             break;
        
        case 20: /** jmp Z -> PC = Z
             */

             pc = mar;
             break;

        case 21: /** ld rX, Z -> rX = *Z
            o que faz: carrega no registrador rX a palavra de memória que está no endereço memoria[Z]
            o endereço de memória vai pro mar
            a palavra que está em mbr será toda sobrescrita pela palavra em memória[mar]
                isso acontece porque o C, quando fazemos uma atribuição de uma variável menor (memoria[mar]) para uma variável maior (mbr)
                ele realiza um zero padding (preenchimentos com zero à esquerda), que transforma o mbr todo em [0000] [0000] [0000] [0000]
            */

            mbr = memoria[mar];
            mbr = (mbr << 8) + memoria[mar + 1];
            reg[ro0] = mbr;

            pc = pc + 3;

            break;
        
        case 22: /** st rX, Z -> *Z = rX
            o que faz: armazena no endereço memoria[Z] a palavra que está no registrador rX
            a palavra que queremos salvar está no registrador de índice ro0
            precisamos passar pro mbr essa palavra
            como o mar é menor que o mbr, e a palavra de memória tem 16 bits, vamos armazenar ela em duas unidades endereçáveis da memória
                que é mar e mar+1
                no memoria[mar], vamos colocar os 8 bits MSB de mbr. Para isso, deslocamos 16 bits para a esquerda e 24 bits para a direita
                na memoria[mar], vamos colocar os 8 bits LSB de mbr. Para isso, deslocamos 24 bits para a esquerda e 24 bits para a direita
            */
            mbr = reg[ro0];
            memoria[mar] = (mbr << 16) >> 24;
            memoria[mar + 1] = (mbr << 24) >> 24;
            pc = pc + 3;
             
            break;
        
        case 23: /** movi rX, IMM -> rX = IMM
            o que faz: coloca no registrador rX o valor do imediato imm
            */
            reg[ro0] = imm;

            pc = pc + 3;

            break;
        
        case 24: /** addi rX, IMM -> rX = rX + imm
            o que faz: soma o imm ao valor de rX e guarda em rX
            */

            reg[ro0] += imm;

            pc = pc + 3;

            break;
        
        case 25: /** subi rX, IMM -> rX = rX - imm
             o que faz: subtrai o imm ao valor de rX e guarda em rX
            */

            reg[ro0] = reg[ro0] - imm;

            pc = pc + 3;

            break;
        
        case 26: /** muli rX, IMM -> rX = rX * imm
             o que faz: multiplica o valor de rX por imm e guarda em rX
            */

            reg[ro0] = reg[ro0] * imm;

            pc = pc + 3;

            break;
        
        case 27: /** divi rX, IMM -> rX = rX / imm
             o que faz: divide o valor de rX por imm e guarda em rX
             */

             reg[ro0] = reg[ro0] / imm;

             pc = pc + 3;

             break;

        case 28: /**lsh rX, IMM -> rX = rX << imm
             o que faz: desloca rX imm bits para a esquerda
             */

             reg[ro0] = reg[ro0] << imm;

             pc = pc + 3;

             break;
        
        case 29: /** rsh rX, IMM -> rX = rX >> imm
             o que faz: desloca rX imm bits para a direita
             */

             reg[ro0] = reg[ro0] >> imm;
             pc = pc + 3;

             break;
        
        default: /** se não for nenhum dos opcodes acima, é porque é uma instrução que não foi codificada
             não devemos executá-la, então vamos mandar uma mensagem de erro
            */
            return 1;

    }

    imprimirEstado();

    return 0;

}

void imprimirEstado(){
    printf("\nCPU:\n");
    // %04X imprime o número em hexadecimal, maiúsculo, com 4 dígitos e preenchidos com 0's 
    // aplicamos o fundo amarelo só para os valores 
    printf("R0:      " BG_YELLOW "%04X" RESET "    R1:      " BG_YELLOW "%04X" RESET "    R2:      " BG_YELLOW "%04X" RESET "    R3: " BG_YELLOW "%04X" RESET "\n", reg[0], reg[1], reg[2], reg[3]);      
    printf("R4:      " BG_YELLOW "%04X" RESET "    R5:      " BG_YELLOW "%04X" RESET "    R6:      " BG_YELLOW "%04X" RESET "    R7: " BG_YELLOW "%04X" RESET "\n", reg[4], reg[5], reg[6], reg[7]);
    printf("MBR:     " BG_YELLOW "%08X" RESET "        MAR:     " BG_YELLOW "%04X" RESET "    IMM:     " BG_YELLOW "%04X" RESET "    PC: " BG_YELLOW "%04X" RESET "\n", mbr, mar, imm, pc);                  
    printf("IR:      " BG_YELLOW "%02X" RESET "              RO0:     " BG_YELLOW "%X" RESET "                RO1: " BG_YELLOW "%X" RESET "\n", ir, ro0, ro1);          
    printf("E:       " BG_YELLOW "%X" RESET "               L:       " BG_YELLOW "%X" RESET "                G:   " BG_YELLOW "%X" RESET "\n", e, l, g);  

    printf("\nMemória:\n");
    // imprime o cabeçalho (colunas) da memória que vai de 00 até 0F
    printf("   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
        // fazemos um for que percorre todas as 256 posições do vetor de memória
        for (int i = 0; i < 256; i += 16) { // para imprimir cada linha em hexadecimal também
        printf("%02X ", i);
        for (int j = 0; j < 16; j++) {
            printf(BG_YELLOW "%02X" RESET " ", memoria[i + j]); // imprime o que está armazenado em cada índice do vetor de memória 
        }
        printf("\n");
    }

    printf("\nPressione uma tecla para iniciar o próximo ciclo de máquina ou aperte CTRL+C para finalizar a execução do trabalho.\n");
    getchar();

    #undef RESET

}   
