# Instruções de Uso: Simulador de Máquina von Neumann
Este documento detalha os passos necessários para compilar o código, interagir com o simulador e carregar novos arquivos de instrução na memória.

## 1. Compilação e Execução
O projeto foi desenvolvido e testado para o ambiente Linux (Ubuntu). Para compilar o código-fonte, abra o seu terminal na raiz do projeto e execute o comando abaixo utilizando o compilador GCC:

Bash
`gcc main.c loader.c -o simulador`

Após a compilação ser concluída sem erros, inicie o simulador com o comando:
Bash
`./simulador`

## 2. Interação com o Simulador
Uma vez executado, o simulador operará de forma interativa, demonstrando passo a passo o funcionamento do ciclo de máquina exigido pelo trabalho:

* **Estado Inicial (Boot):** O programa iniciará exibindo o estado da CPU com todos os registradores e o vetor de memória de 256 posições totalmente zerados.

* **Interface Visual:** Para facilitar a leitura e o acompanhamento da arquitetura, os valores numéricos impressos no terminal (registradores e matriz de memória) possuem um fundo amarelo de destaque.

* **Avanço de Ciclo:** A simulação é controlada pelo usuário. Conforme indicado na tela, basta pressionar a tecla Enter para avançar a execução do processador. A cada "Enter", a CPU e a memória serão impressas novamente, refletindo o novo estado após a conclusão daquele ciclo de instrução.

## 3. Como Rodar um Novo Programa
Para simular um código Assembly diferente do arquivo padrão, siga exatamente os passos abaixo:
* Crie um novo arquivo de texto (ex: `novo_programa.txt`) na pasta raiz do projeto.
* Abra o arquivo `main.c`.
* Vá até a linha 37 e altere o nome do arquivo dentro da chamada da função `carregar_memoria()`.
    *Exemplo:* `carregar_memoria("novo_programa.txt", memoria);`

* Salve o arquivo main.c e recompile o projeto inteiramente (repetindo o passo 1 da seção de Compilação).

## 4. Estrutura Obrigatória do Arquivo .txt
O arquivo de texto contendo o seu programa deve seguir rigorosamente a formatação exigida pelo Montador (Loader). O arquivo é dividido em três partes: endereço;tipo;instrução_ou_dado.

Abaixo está a estrutura de exemplo validada e pronta para uso:

```txt
0;i; movi r0, 0
3;i; movi r1, a
6;i; ld r3, fe
9;i; ld r2, fc
c;i; str r3, r2
e;i; addi r2, 2
11;i; st r2, fc
14;i; addi r0, 1
17;i; cmp r0, r1
19;i; jl c
1c;i; hlt
fc;d; 0030
fe;d; ffff
```
### WARNING IMPORTANTE - SINTAXE DO ARQUIVO
**NÃO PODE HAVER ESPAÇOS** entre os pontos e vírgulas (`;`) na declaração das linhas. O Montador foi programado para ler os delimitadores exatos. Inserir espaços (*exemplo errado:* `0 ; i ; movi r0, 0`) fará com que o arquivo não seja lido corretamente e o programa não irá rodar.