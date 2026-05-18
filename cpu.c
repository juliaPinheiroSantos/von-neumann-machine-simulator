#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    // formato de inst: [opcode | reg0 | reg1 | 0 ] (16 bits)
    if (ir >= 2 && ir <= 12){
        ro0 = (mbr << 13) >> 29; 
        // [rrr00000] [00000000] [00000000] [00000000] (<< 13) -> removemos campo opcode
        // [00000000] [00000000] [00000000] [00000rrr] (>> 29), 32 - 29 = 3 (tam reg)
        ro1 = (mbr << 16) >> 29; // tira o campo opcode + ro0 depois desloca 29 bits à direita
    } else if(ir == 13){
        // not (8 bits)
        ro0 = (mbr << 13) >> 29;
    } else if(ir >= 14 && ir <= 20){
        // inst [ opcode | 0 | mem adress ] (24 bits)
        imm = (mbr << 8) >> 16; // extrai endereço de memória (8 bits)
    } else if(ir >= 21 && ir <= 29){
        // inst [ opcode | reg0 | mem adress ] (24 bits)
        ro0 = (mbr << 13) >> 29; // extrai primeiro registrador
        imm = (mbr << 16) >> 16; // extrai endereço de memória ou imediato (16 bits)
    } else {
        // formato de inst: [ opcode | 0 ] (8 bits)
        // hlt & nop -> fazem nada
    }
}

void imprime_estado(){
    printf("\n");
    printf("CPU:\n");
    printf("R0: %04X R1: %04X R2: %04X R3: %04X\n", reg[0], reg[1], reg[2], reg[3]);
    printf("R4: %04X R5: %04X R6: %04X R7: %04X\n", reg[4], reg[5], reg[6], reg[7]);
    printf("MBR: %08X MAR: %04X IMM: %04X PC: %04X\n", mbr, mar, imm, pc);
    printf("IR: %02X RO0: %X RO1: %X\n", ir, ro0, ro1);
    printf("E: %X L: %X G: %X\n", e, l, g);
    printf("\nMemória:\n");
    printf("   ");
    for(int i = 0; i < 16; i++){
        printf(" %02X", i);
    }
    printf("\n");
    for(int i = 0; i < 256; i += 16){
        printf("%02X ", i);
        for(int j = 0; j < 16; j++){
            printf(" %02X", memoria[i + j]);
        }
        printf("\n");
    }
    printf("\nPressione uma tecla para iniciar o próximo ciclo de máquina ou aperte CTRL+C para finalizar a execução do trabalho.\n");
    getchar();
}

void le_arquivo(char *nome_arquivo){
    FILE *fp = fopen(nome_arquivo, "r");
    if(fp == NULL){
        printf("Erro ao abrir arquivo %s\n", nome_arquivo);
        return;
    }

    char linha[100];
    unsigned short int endereco;
    char tipo;
    unsigned short int valor;

    // inicializa memória com 0xFF
    for(int i = 0; i < 256; i++){
        memoria[i] = 0xFF;
    }

    while(fgets(linha, sizeof(linha), fp)){
        // formato: endereco;tipo;valor
        // tipo 'i' = instrução, tipo 'd' = dado
        if(strcmp(linha, "\n") == 0 || strcmp(linha, "") == 0) continue;

        if(sscanf(linha, "%hx;%c;%hx", &endereco, &tipo, &valor) == 3){
            if(tipo == 'i'){
                // instrução - pode ter 1, 2 ou 3 bytes
                // verificar tamanho da instrução pela leitura
                unsigned char opcode = (valor >> 5) & 0x1F;
                
                if(opcode == 0 || opcode == 1){
                    // hlt e nop - 1 byte
                    memoria[endereco] = (valor >> 8) & 0xFF;
                } else if(opcode >= 2 && opcode <= 13){
                    // 2 registradores - 2 bytes
                    memoria[endereco] = (valor >> 8) & 0xFF;
                    memoria[endereco + 1] = valor & 0xFF;
                } else if(opcode >= 14 && opcode <= 20){
                    // endereço de memória - 3 bytes
                    memoria[endereco] = (valor >> 16) & 0xFF;
                    memoria[endereco + 1] = (valor >> 8) & 0xFF;
                    memoria[endereco + 2] = valor & 0xFF;
                } else if(opcode >= 21 && opcode <= 29){
                    // registrador + endereço/imediato - 3 bytes
                    memoria[endereco] = (valor >> 16) & 0xFF;
                    memoria[endereco + 1] = (valor >> 8) & 0xFF;
                    memoria[endereco + 2] = valor & 0xFF;
                }
            } else if(tipo == 'd'){
                // dado - sempre 2 bytes
                memoria[endereco] = (valor >> 8) & 0xFF;
                memoria[endereco + 1] = valor & 0xFF;
            }
        } else if(strstr(linha, "hlt") != NULL){
            break; // encontrou hlt, para de ler
        }
    }

    fclose(fp);
}

void executa(){
    // executa a instrução de acordo com o opcode em ir usando if e else if
    unsigned char byte_alto, byte_baixo;

    if (ir == 0) { // hlt
        printf("Instrução: HLT\n");
    } else if (ir == 1) { // nop
        printf("Instrução: NOP\n");
        pc += 1;
    } else if (ir == 2) { // ldr rX, rY - rX = *rY
        printf("Instrução: LDR R%d, R%d\n", ro0, ro1);
        mar = reg[ro1];
        mbr = memoria[mar];
        byte_alto = mbr;
        mar++;
        byte_baixo = memoria[mar];
        reg[ro0] = (byte_alto << 8) | byte_baixo;
        pc += 2;
    } else if (ir == 3) { // str rX, rY - *rY = rX
        printf("Instrução: STR R%d, R%d\n", ro0, ro1);
        mar = reg[ro1];
        byte_alto = (reg[ro0] >> 8) & 0xFF;
        byte_baixo = reg[ro0] & 0xFF;
        memoria[mar] = byte_alto;
        mar++;
        memoria[mar] = byte_baixo;
        pc += 2;
    } else if (ir == 4) { // add rX, rY - rX = rX + rY
        printf("Instrução: ADD R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] + reg[ro1];
        pc += 2;
    } else if (ir == 5) { // sub rX, rY - rX = rX - rY
        printf("Instrução: SUB R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] - reg[ro1];
        pc += 2;
    } else if (ir == 6) { // mul rX, rY - rX = rX * rY
        printf("Instrução: MUL R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] * reg[ro1];
        pc += 2;
    } else if (ir == 7) { // div rX, rY - rX = rX / rY
        printf("Instrução: DIV R%d, R%d\n", ro0, ro1);
        if(reg[ro1] != 0){
            reg[ro0] = reg[ro0] / reg[ro1];
        }
        pc += 2;
    } else if (ir == 8) { // cmp rX, rY
        printf("Instrução: CMP R%d, R%d\n", ro0, ro1);
        e = (reg[ro0] == reg[ro1]) ? 1 : 0;
        l = (reg[ro0] < reg[ro1]) ? 1 : 0;
        g = (reg[ro0] > reg[ro1]) ? 1 : 0;
        pc += 2;
    } else if (ir == 9) { // movr rX, rY - rX = rY
        printf("Instrução: MOVR R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro1];
        pc += 2;
    } else if (ir == 10) { // and rX, rY - rX = rX & rY
        printf("Instrução: AND R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] & reg[ro1];
        pc += 2;
    } else if (ir == 11) { // or rX, rY - rX = rX | rY
        printf("Instrução: OR R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] | reg[ro1];
        pc += 2;
    } else if (ir == 12) { // xor rX, rY - rX = rX ^ rY
        printf("Instrução: XOR R%d, R%d\n", ro0, ro1);
        reg[ro0] = reg[ro0] ^ reg[ro1];
        pc += 2;
    } else if (ir == 13) { // not rX - rX = !rX
        printf("Instrução: NOT R%d\n", ro0);
        reg[ro0] = ~reg[ro0];
        pc += 1;
    } else if (ir == 14) { // je Z - jump if equal
        printf("Instrução: JE %04X\n", imm);
        if(e == 1){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 15) { // jne Z - jump if not equal
        printf("Instrução: JNE %04X\n", imm);
        if(e == 0){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 16) { // jl Z - jump if lower
        printf("Instrução: JL %04X\n", imm);
        if(l == 1){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 17) { // jle Z - jump if lower or equal
        printf("Instrução: JLE %04X\n", imm);
        if(e == 1 || l == 1){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 18) { // jg Z - jump if greater
        printf("Instrução: JG %04X\n", imm);
        if(g == 1){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 19) { // jge Z - jump if greater or equal
        printf("Instrução: JGE %04X\n", imm);
        if(e == 1 || g == 1){
            pc = imm;
        } else {
            pc += 3;
        }
    } else if (ir == 20) { // jmp Z - jump
        printf("Instrução: JMP %04X\n", imm);
        pc = imm;
    } else if (ir == 21) { // ld rX, Z - rX = *Z
        printf("Instrução: LD R%d, %04X\n", ro0, imm);
        mar = imm;
        mbr = memoria[mar];
        byte_alto = mbr;
        mar++;
        byte_baixo = memoria[mar];
        reg[ro0] = (byte_alto << 8) | byte_baixo;
        pc += 3;
    } else if (ir == 22) { // st rX, Z - *Z = rX
        printf("Instrução: ST R%d, %04X\n", ro0, imm);
        mar = imm;
        byte_alto = (reg[ro0] >> 8) & 0xFF;
        byte_baixo = reg[ro0] & 0xFF;
        memoria[mar] = byte_alto;
        mar++;
        memoria[mar] = byte_baixo;
        pc += 3;
    } else if (ir == 23) { // movi rX, IMM - rX = IMM
        printf("Instrução: MOVI R%d, %04X\n", ro0, imm);
        reg[ro0] = imm;
        pc += 3;
    } else if (ir == 24) { // addi rX, IMM - rX = rX + IMM
        printf("Instrução: ADDI R%d, %04X\n", ro0, imm);
        reg[ro0] = reg[ro0] + imm;
        pc += 3;
    } else if (ir == 25) { // subi rX, IMM - rX = rX - IMM
        printf("Instrução: SUBI R%d, %04X\n", ro0, imm);
        reg[ro0] = reg[ro0] - imm;
        pc += 3;
    } else if (ir == 26) { // muli rX, IMM - rX = rX * IMM
        printf("Instrução: MULI R%d, %04X\n", ro0, imm);
        reg[ro0] = reg[ro0] * imm;
        pc += 3;
    } else if (ir == 27) { // divi rX, IMM - rX = rX / IMM
        printf("Instrução: DIVI R%d, %04X\n", ro0, imm);
        if(imm != 0){
            reg[ro0] = reg[ro0] / imm;
        }
        pc += 3;
    } else if (ir == 28) { // lsh rX, IMM - rX = rX << IMM
        printf("Instrução: LSH R%d, %04X\n", ro0, imm);
        reg[ro0] = reg[ro0] << imm;
        pc += 3;
    } else if (ir == 29) { // rsh rX, IMM - rX = rX >> IMM
        printf("Instrução: RSH R%d, %04X\n", ro0, imm);
        reg[ro0] = reg[ro0] >> imm;
        pc += 3;
    } else {
        printf("Instrução desconhecida: %02X\n", ir);
        pc += 1;
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Uso: %s <arquivo_programa>\n", argv[0]);
        return 1;
    }

    // inicializa registradores e flags
    for(int i = 0; i < 8; i++){
        reg[i] = 0xFFFF;
    }
    mbr = 0xFFFFFFFF;
    mar = 0xFFFF;
    pc = 0x0000;
    imm = 0xFFFF;
    ir = 0xFF;
    ro0 = 0xF;
    ro1 = 0xF;
    e = 0xF;
    l = 0xF;
    g = 0xF;

    // carrega o arquivo
    le_arquivo(argv[1]);

    // exibe estado inicial
    imprime_estado();

    // ciclo de máquina
    int halt = 0;
    while(!halt){
        busca();
        decodifica();
        imprime_estado();
        executa();
        
        // verifica se foi hlt
        if(ir == 0){
            halt = 1;
        }
    }

    printf("\nPrograma finalizado.\n");
    return 0;
}