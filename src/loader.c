#include <stdio.h>
#include <string.h>
#include "loader.h"

/** Arquivo "loader" ou montador, responsável por carregar nosso programa.txt na memória.
 */


/**
 * @brief Struct instrucao
    Para facilitar, criamos uma struct instrucao que vai "catalogar" todos os opcodes que temos junto com o número dele.
    char mnemonico[10] vai armazenar o comando, como hlt, ldr e etc.
    unsigned char opcode vai ser o opcode de cada instrução em decimal. 
        Colocamos como char porque vamos comparar com stcmp() com o char que vier na leitura de cada linha.
 * 
 */
typedef struct {
    char mnemonico[10];
    unsigned char opcode;
} instrucao;


/**
 * @brief a tabela com todas as instruções catalogadas
 * 
 */
instrucao tabela_instrucoes[] = {
    {"hlt", 0}, {"nop", 1}, {"ldr", 2}, {"str", 3}, {"add", 4},
    {"sub", 5}, {"mul", 6}, {"div", 7}, {"cmp", 8}, {"movr", 9},
    {"and", 10}, {"or", 11}, {"xor", 12}, {"not", 13}, {"je", 14},
    {"jne", 15}, {"jl", 16}, {"jle", 17}, {"jg", 18}, {"jge", 19},
    {"jmp", 20}, {"ld", 21}, {"st", 22}, {"movi", 23}, {"addi", 24},
    {"subi", 25}, {"muli", 26}, {"divi", 27}, {"lsh", 28}, {"rsh", 29}
};



/**
 * @brief É a função responsável por comparar o mnemonico que está no programa.txt com o mnemonico da tabela
    Verifica todos os mnemonicos (ao todo 30) até achar. Quando achar, entra no if e retorna o número do opcode.
    Se não achar, é porque esse mnemonico lido não existe no nosso conjunto de instruções. Retorna -1.
 * 
 * @param mnemonico lido do arquivo txt
 * @return int retorna ou o código do mnemônico de acordo com a tabela ou -1 quando ele não existir na tabela
 */
int obter_opcode(char *mnemonico){
    for (int i = 0; i < 30; i++) {
        if(strcmp(mnemonico, tabela_instrucoes[i].mnemonico) == 0){
            return tabela_instrucoes[i].opcode;
        }
    }

    return -1;
}



/**
 * @brief É a função responsável por ler linha por linha do programa.txt e carregar ele na memória.
 *  
    @details Uso da função sscanf
    sscanf vai fatiar a linha baseada nos ';'
    o operador %x lê o endereço em hexdecimal
    o operador %c lê o caracter 'i' (instrução) ou 'd' (dado)
        o operador %[^\n] vai ler tudo o que vier depois do ';' depois de i ou d, até a quebra de linha '\n'
        %[] é um scanset que serve para ler uma sequência de string que contenha apenas os caracteres especificados dentro dele
        mas quando usamos o operador '^', nós "negamos" isso -> ou seja, ele vai ter todos os caracteres menos o '\n'
            isso faz com que ele leia a linha toda até achar uma quebra de linha
        se o nosso scscanf conseguiu achar os 3 campos (endereco, tipo e conteúdo) é porque ele conseguiu ler a linha toda de instrução/dado
                por isso fazemos o if (sscanf(..) == 3), porque o sccanf é uma função da biblioteca string.h que retorna quantas "fatias" ela fez na linha
    
 * @param arq é o arquivo txt
 * @param memoria passamos o vetor de memoria[256] como parâmetro para a função já realizar o salvamento do programa em suas células.
 */
void carregar_memoria(const char *arq, unsigned char *memoria) {
    //padrão pra leitura de arquivos em C
    FILE *arquivo = fopen(arq, "r");
    if (arquivo == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo %s\n", arq);
        return;
    }

    // deixamos uma linha com no máximo 100 caracteres. essa variável vai guardar a linha que está sendo lida temporariamente.
    char linha[100];                                                
    
    //lê o arquivo linha por linha até o final
    while (fgets(linha, sizeof(linha), arquivo)) {
        //criamos as variáveis antes para ficar mais fácil de manipular quando as "fatiarmos"
        unsigned int endereco;
        char tipo;
        char conteudo[50];
        unsigned int dado;
        char mnemonico[10];
        int rx = 0, ry = 0;
        unsigned int z = 0;
        int opcode = 0;

        if (sscanf(linha, "%x;%c;%[^\n]", &endereco, &tipo, conteudo) == 3) {
            // se o tipo da linha for um dado
            if(tipo == 'd'){
                //essa função sscanf também consegue converter um texto para um tipo de variável que a gente precisa.
                    //nesse caso, ela está pegando o conteúdo, que é um dado, e transformando em um número hexadecimal com '%x', e armazenando esse valor na variável 'dado'
                sscanf(conteudo, "%x", &dado);              
                // como os dados aqui tem 16 bits, vamos precisar manipular os bits dele para caber na memória.
                    // os 8 bits MSB ficam na posição memoria[endereço] e os outros 8bits LSB ficam na posição memoria[endereco + 1]
                memoria[endereco] = dado >> 8;
                memoria[endereco + 1] = (dado << 8) >> 8;
            } else if(tipo == 'i'){ //se o tipo da linha for uma instrução
                // converte o conteúdo para string e armazena na variável 'mnemonico'
                sscanf(conteudo, "%s", mnemonico);
                //achamos qual é o nosso opcode de acordo com a nossa catalogação do início e fazemos isso com a função obter_opcode
                opcode = obter_opcode(mnemonico);

                /** aqui só preenchemos a memória de acordo com o tamanho da palavra de instrução
                    para as instruções que tem o mesmo tamanho e mesmo estrutura, que são de 2-12, 14-20 e 21-29, nós vamos "agrupar" as cases
                        fazemos esse agrupamento usando {}, porque o C permite fall-through
                 */
                if(opcode == -1){
                    continue;
                }

                switch(opcode){
                    case 0:
                    case 1: 
                        memoria[endereco] = opcode << 3;
                        break;
                    
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        {
                            sscanf(conteudo, "%*s r%d, r%d", &rx, &ry);
                            memoria[endereco] = (opcode << 3) | rx;
                            memoria[endereco + 1] = (ry << 5);
                            break;
                        }
                    
                    case 13:
                        sscanf(conteudo, "%*s r%d", &rx);
                        memoria[endereco] = (opcode << 3) | rx;
                        break;
                    
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    case 20:
                        {
                            sscanf(conteudo, "%*s %x", &z);
                            memoria[endereco] = opcode << 3;
                            memoria[endereco + 1] = z >> 8;
                            memoria[endereco + 2] = (z << 8) >> 8;
                            break;
                        }

                    case 21:
                    case 22:
                    case 23:
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                    case 28:
                    case 29:
                        {
                            sscanf(conteudo, "%*s r%d, %x", &rx, &z);
                            memoria[endereco] = (opcode << 3) | rx;
                            memoria[endereco + 1] = z >> 8;
                            memoria[endereco + 2] = (z << 8) >> 8;
                            break;
                        }
                    
                    default: 
                            //esse erro só acontece se der algum problema interno não identificado
                            // pois se a instrução lida não existir no nosso conjunto catalogado, isso já vai ser notificado na função obter_opcode
                            printf("Erro no loader");
                
                }
            }
        }
    }

    fclose(arquivo);
}
