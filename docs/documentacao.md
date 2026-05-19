# Documentação do Simulador

## 1. Estruturas de Dados

### `struct instrucao`
*(Defined in loader.c:17)*

Para facilitar, criamos uma struct instrucao que vai "catalogar" todos os opcodes que temos junto com o número dele. O arquivo "loader" ou montador, é o responsável por carregar nosso programa.txt na memória.

* `char mnemonico[10]`: vai armazenar o comando, como hlt, ldr e etc. Colocamos como char porque vamos comparar com stcmp() com o char que vier na leitura de cada linha.
* `unsigned char opcode`: vai ser o opcode de cada instrução em decimal.

---

## 2. Variáveis Globais (Registradores e Memória)

| Tipo | Variável | Descrição / Valor |
| :--- | :--- | :--- |
| `unsigned int` | **mbr** | Memory Buffer Register |
| `unsigned short int` | **mar** | Memory Address Register |
| `unsigned short int` | **pc** | Program Counter |
| `unsigned short int` | **imm** | Imediato |
| `unsigned char` | **ir** | Instruction Register |
| `unsigned char` | **ro0** | Registrador de Operando 0 |
| `unsigned char` | **ro1** | Registrador de Operando 1 |
| `unsigned char` | **e** | Flag Equal |
| `unsigned char` | **l** | Flag Less |
| `unsigned char` | **g** | Flag Greater |
| `unsigned short` | **reg** | Array de registradores de propósito geral |
| `unsigned char` | **memoria** | Array da memória `{0}` |
| `instrucao` | **tabela_instrucoes** | A tabela com todas as instruções catalogadas: `{"hlt", 0}`, `{"nop", 1}`, `{"ldr", 2}`, `{"str", 3}`, `{"add", 4}`, `{"sub", 5}`, `{"mul", 6}`, `{"div", 7}`, `{"cmp", 8}`, `{"movr", 9}`, `{"and", 10}`, `{"or", 11}`, `{"xor", 12}`, `{"not", 13}`, `{"je", 14}`, `{"jne", 15}`, `{"jl", 16}`, `{"jle", 17}`, `{"jg", 18}`, `{"jge", 19}`, `{"jmp", 20}`, `{"ld", 21}`, `{"st", 22}`, `{"movi", 23}`, `{"addi", 24}`, `{"subi", 25}`, `{"muli", 26}`, `{"divi", 27}`, `{"lsh", 28}`, `{"rsh", 29}` |

---

## 3. Funções do Ciclo de Máquina

### `void fetch()`
**Função de Busca**
Responsável por buscar o próximo endereço de memória e armazenar no registrador mbr. Assumimos que cada instrução poderá ter 1, 2 ou 3 (bytes). Cada endereço de memória armazena 1 byte (8 bits). Vamos buscar sempre 3 bytes e inserir no mbr através de operações bit-a-bit de "shift".

### `void decode()`
**Função de Decodificação**
Responsável por "fatiar" a palavra de memória buscada e distribuir cada "fatia" para um registrador designado. O estado atual do registrador mbr é: `[0000 0000] [byte 1] [byte 2] [byte 3]`. As instruções podem ser de 5 tipos diferentes: 
* `[opcode | 0]` 
* `[opcode | reg0 | reg 1 | 0]` 
* `[opcode | reg0 ]` 
* `[opcode | 0 | endereço de memória]` 
* `[opcode | reg0 | endereço de memória ou valor imediato]`

Ou seja, quando buscamos sempre 3 bytes na função fetch(), pode sim acontecer de, ao buscar a instrução nop que tem esse formato: `[opcode | 0]`, tenhamos no mbr as seguintes informações `[0000 0000] [0000 1000] [0001 0000] [0000 0000]` onde: 000010 é opcode de nop, os 000 seguintes faz parte dessa instrução 00100 é o opcode de add, os 000 seguintes indicam o índice do 1º registrador dessa instrução, e os 000 seguintes a estes indicam o índice do 2º registrador dessa instrução. 

Usar o PC (program counter) resolve isso para nós: a cada ciclo de busca-decodifica-executa, o pc é incrementado para a próxima posição de memória que possui uma instrução. Então, se nop esta no endereço 10, o PC conterá 10 até o final desse ciclo de instrução. No próximo ciclo, ao ser incrementado, ele passa a apontar para a posição 11, que indica a instrução add seguinte. Mesmo que no mbr quando pc = 10 tinha parte da próxima instrução, essa parte é "descartada" e "lida novamente" no próximo ciclo.

O que cada registrador vai conter? 
Mbr (em todas as instruções) - contém a palavra de instrução inteira `[0000 0000] [0101 1111] [1111 1111] [0000 0000]` como queremos somente os bits de 23:19 referentes ao opcode, deslocamos para a direita em 19 bits. `[0000 0000] [0101 1111] [1111 1111] [0000 0000]` -> `[0000 0000] [0000 0000] [0000 0000] [0000 1011]`

* **Instruções com 8 bits - hlt (0) e nop (1):** formato: `[0000 0000] [opcode(5bits) 000] [8bits outra instrução] [8bits outra instrução]` ambas não fazem "nada" no decode(), então só pegamos o opcode delas e guardamos em ir.
* **Instruções com 16 bits - ldr (2) até xor (12):** formato: `[0000 0000] [opcode(5bits) reg0(3bits)] [reg1(3bits) 0 0000] [8bits outra instrução]` todas guardam os índices dos registradores em reg0 (1º registrador da instrução) e em reg1 (2º registrador da instrução) para pegar reg0 (ou ro0), precisamos deslocar 13bits para a esquerda e depois 29bits para a direita para pegar reg1 (ou ro1), precisamos deslocar 16bits para a esquerda e depois 29bits para a direita.
* **Instruções com 8 bits - not (13):** formato: `[0000 0000] [opcode(5bits) reg0(3bits)] [8bits outra instrução] [8bits outra instrução]` o índice do registrador da instrução precisa ficar no reg0 (ou ro0) para pegar o reg0, precisamos deslocar 13bits para a esquerda e depois 29bits para a direita.
* **Instrução com 24 bits - je (14) até jmp (20):** formato: `[0000 0000] [opcode(5bits) 000] [endereço de memória(8bits)] [endereço de memória(8bits)]` o endereço de memória precisa ficar no registrador PC para pegar o endereço de memória, precisamos deslocar 16 bits para a esquerda e depois 16 bits para a direita.
* **Instrução com 24 bits - ld (21) até st (22):** formato: `[0000 0000] [opcode(5bits) reg0(3bits)] [endereço de memória(8bits)] [endereço de memória(8bits)]` o índice do registrador da instrução precisa ficar no reg0 (ou ro0) para pegar o reg0, precisamos deslocar 13 bits para a esquerda e depois 29 bits para a direita o endereço de memória precisa ficar no registrador mar para pegar o endereço de memória, precisamos deslocar 16bits para a esquerda e depois 16bits para a direita.
* **Instrução com 24 bits - movi(23) até rsh (29):** formato: `[0000 0000] [opcode(5bits) reg0(3bits)] [endereço de memória(8bits)] [endereço de memória(8bits)]` o índice do registrador da instrução precisa ficar no reg0 (ou ro0) para pegar o reg0, precisamos deslocar 13 bits para a esquerda e depois 29 bits para a direita o imediato precisa ficar no registrador imm para pegar o imediato, precisamos deslocar 16bits para a esquerda e depois 16bits para a direita.

### `int execute()`
**Função de Execução**
Responsável por executar a instrução de acordo com o seu opcode (armazenado no registrador ir). O estado atual de cada registrador depende: da instrução buscada anteriormente e da instrução buscada no atual ciclo de máquina. O incremento do pc depois da execução de cada instrução depende do tamanho da instrução que foi executada: se a instrução tem 1 byte, incrementa o pc em 1; se a instrução tem 2 bytes, incrementa o pc em 2; se a instrução tem 3 bytes, incrementa o pc em 3. Todo endereço de memória deve ser indicado pelo mar. Todo o tráfego de e para a memória RAM deve passar pelo mbr.

**Retorno:** Retorna `int` - condição de parada do while na função main.

**Cases:**
* **hlt (0):** O que faz: para o programa. Como o nosso ciclo de máquina continua funcionando while(flag == 0), retornar 1 nesse case vai fazer o ciclo parar.
* **nop (1):** O que faz: nenhuma operação, só incremento do registrador pc.
* **ldr rX, rY (2):** O que faz: carrega no registrador rX o conteúdo do endereço indicado pelo registrador rY. Se queremos pegar o conteúdo do endereço indicado por rY, precisamos que o endereço desse conteúdo esteja no mar além disso, o conteúdo depois de buscado, precisa estar no mbr depois disso é que esse conteúdo deve ser inserido no registrador indicado por rX.
* **str rX, rY (3):** O que faz: armazena no endereço indicado por rY o valor do registrador rX. Se queremos pegar o endereço de rY (que é reg[ro1]), precisamos que esse endereço esteja no mar. Se queremos armazenar o conteúdo de rX (que é reg[ro0]) no endereço de rY, precisamos que o conteúdo de rX esteja no mbr.
* **add rX, rY (4):** O que faz: adiciona o conteúdo de rX e rY e guarda esse valor em rX.
* **sub rX, rY (5):** O que faz: subtrái do conteúdo de rX o valor de rY, e guarda esse valor em rX.
* **mul rX, rY (6):** O que faz: multiplica o conteúdo de rX pelo conteúdo de rY, e guarda esse valor em rX.
* **div rX, rY (7):** O que faz: divide o conteúdo de rX pelo conteúdo de rY, e guarda esse valor em rX. Não precisamos nos preocupar com um resultado de "valor flutuante" no caso dessa divisão de inteiros, pois o C já fica responsável por ignorar o resto e considerar só o inteiro. Entretanto, no caso de divisão por 0, isso pode dar algum erro.
* **cmp rX, rY (8):** O que faz: compara o conteúdo de rX com o conteúdo de rY. Se rX = rY, então e = 1; senão, e = 0. Se rX < rY, então l = 1; senão, l = 0. Se rX > rY, então g = 1; senão g = 0.
* **movr rX, rY (9):** O que faz: substitui o valor de rX pelo valor de rY, é uma atribuição simples.
* **and rX, rY (10):** O que faz: operação bit-a-bit. Realiza a operação lógica '&' entre rX e rY e guarda o resultado dessa operação em rX. O operaçao '&' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna. Se nessa coluna ambos os bits forem 1, ele resulta em 1. Se um deles for 0, resulta em 0.
* **or rX, rY (11):** O que faz: operação bit-a-bit. Realiza a operação lógica '\|' (or) entre rX e rY e guarda o resultado dessa operação em rX. A operação '\|' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna. Se nessa coluna um dos bits for 1, ele resulta em 1. Se ambos os bits forem 0, resulta em 0.
* **xor rX, rY (12):** O que faz: operação bit-a-bit. Realiza a operação lógica '^' (xor) entre rX e rY e guarda o resultado dessa operação em rX. A operação '^' alinha os dois números binários um debaixo do outro e faz uma comparação coluna por coluna. Se nessa coluna os bits forem diferentes entre si, ele resulta em 1. Se forem iguais, resulta em 0.
* **not rX (13):** O que faz: operação bit-a-bit. Ele inverte todos os bits do valor armazenado em rx. O que é 1 vira 0, e o que é 0 vira 1. Em C, o operador que faz isso é o '~'.
* **je Z (14):** O que faz: salto para o endereço Z se a flag E = 1. O endereço Z vai estar armazendo no registrador mar não pode incrementar o pc para a próxima posição porque vai ir para a posição Z que está armazenada em PC agora se não entrar na condição, deve buscar a próxima instrução, já que não vai pro endereço Z do salto.
* **jne Z (15):** O que faz: salto para o endereço Z se a flag E = 0.
* **jl Z (16):** O que faz: salto para o endereço Z se a flag L = 1.
* **jle Z (17):** O que faz: salto para o endereço Z se a flag E = 1 ou a flag L = 1.
* **jg Z (18):** O que faz: salto para o endereço Z se a flag G = 1.
* **jge Z (19):** O que faz: salto para o endereço Z se a flag E = 1 ou G = 1.
* **jmp Z (20):** O que faz: salto incondicional.
* **ld rX, Z (21):** O que faz: carrega no registrador rX a palavra de memória que está no endereço memoria[Z]. O endereço de memória vai pro mar. A palavra que está em mbr será toda sobrescrita pela palavra em memória[mar]. Isso acontece porque o C, quando fazemos uma atribuição de uma variável menor (memoria[mar]) para uma variável maior (mbr) ele realiza um zero padding (preenchimentos com zero à esquerda), que transforma o mbr todo em [0000] [0000] [0000] [0000].
* **st rX, Z (22):** O que faz: armazena no endereço memoria[Z] a palavra que está no registrador rX. A palavra que queremos salvar está no registrador de índice ro0 precisamos passar pro mbr essa palavra. Como o mar é menor que o mbr, e a palavra de memória tem 16 bits, vamos armazenar ela em duas unidades endereçáveis da memória que é mar e mar+1. No memoria[mar], vamos colocar os 8 bits MSB de mbr. Para isso, deslocamos 16 bits para a esquerda e 24 bits para a direita. Na memoria[mar], vamos colocar os 8 bits LSB de mbr. Para isso, deslocamos 24 bits para a esquerda e 24 bits para a direita.
* **movi rX, IMM (23):** O que faz: coloca no registrador rX o valor do imediato imm.
* **addi rX, IMM (24):** O que faz: soma o imm ao valor de rX e guarda em rX.
* **subi rX, IMM (25):** O que faz: subtrai o imm ao valor de rX e guarda em rX.
* **muli rX, IMM (26):** O que faz: multiplica o valor de rX por imm e guarda em rX.
* **divi rX, IMM (27):** O que faz: divide o valor de rX por imm e guarda em rX.
* **lsh rX, IMM (28):** O que faz: desloca rX imm bits para a esquerda.
* **rsh rX, IMM (29):** O que faz: desloca rX imm bits para a direita.

---

## 4. Funções do Loader e Auxiliares

### `void carregar_memoria(const char * arq, unsigned char * memoria)`
É a função responsável por ler linha por linha do programa.txt e carregar ele na memória. 

Uso da função sscanf: sscanf vai fatiar a linha baseada nos ';'. O operador x lê o endereço em hexdecimal. O operador c lê o caracter 'i' (instrução) ou 'd' (dado). O operador `%[^\n]` vai ler tudo o que vier depois do ';' depois de i ou d, até a quebra de linha '\n'. `%[]` é um scanset que serve para ler uma sequência de string que contenha apenas os caracteres especificados dentro dele, mas quando usamos o operador '^', nós "negamos" isso -> ou seja, ele vai ter todos os caracteres menos o '\n'. Isso faz com que ele leia a linha toda até achar uma quebra de linha. Se o nosso scscanf conseguiu achar os 3 campos (endereco, tipo e conteúdo) é porque ele conseguiu ler a linha toda de instrução/dado. Por isso fazemos o `if (sscanf(..) == 3)`, porque o sccanf é uma função da biblioteca string.h que retorna quantas "fatias" ela fez na linha.

**Parâmetros:**
* `arq` / `nome_arquivo`: é o arquivo txt lido.
* `memoria`: passamos o vetor de memoria[256] como parâmetro para a função já realizar o salvamento do programa em suas células.

### `int obter_opcode(char * mnemonico)`
Esse arquivo é a biblioteca do loader, em que declara o protótipo de todas as funções que loader.c deve implementar. É a função responsável por comparar o mnemonico que está no programa.txt com o mnemonico da tabela. Verifica todos os mnemonicos (ao todo 30) até achar. Quando achar, entra no if e retorna o número do opcode. Se não achar, é porque esse mnemonico lido não existe no nosso conjunto de instruções. Retorna -1.

**Parâmetros:**
* `mnemonico`: lido do arquivo txt.

**Retorno:** * `int`: retorna ou o código do mnemônico de acordo com a tabela ou -1 quando ele não existir na tabela.

### `void imprimirEstado()`
Imprime a matriz de memória e o valor de todos os registradores. `%04X` imprime o número em hexadecimal, maiúsculo, com 4 dígitos e preenchidos com 0's. Aplicamos o fundo amarelo só para os valores com a constante BG_YELLOW. Para resetar quando não queremos aplicar o fundo amarelo, usamos RESET.

A matriz de memória é impressa com dois for's aninhados: O externo para cada linha e o interno para cada célula daquela linha e daquela coluna (impressa anterioremente fora do for com a formatação correta).

### `int main()`
Ponto de entrada do programa.