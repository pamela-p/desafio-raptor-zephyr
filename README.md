# Relatório Técnico: Desafio Sistemas Embarcados (Raptor)


**Autora:** Pâmela da Silva Paes
**Data:** Março/2026


-------------------------------------------------------------------------


## 1. Introdução
Este projeto tem como objetivo elaborar uma solução em ambiente de simulação usando QEMU para o Sistema Operacional de Tempo Real (RTOS) ZephyrOS. O escopo principal consiste em exibir a mensagem "Hello World! This is Raptor!" em ambiente de virtualização. Além do requisito obrigatório, foi desenvolvida uma *feature* adicional utilizando *Message Queues* e uma *Finite State Machine*(FSM) para simular a telemetria, o monitoramento e a lógica estruturada de alertas baseada na pressão de água de um reservatorio.


## 2. Ambiente de Desenvolvimento
PAra o desenvolvimento e simulação, o seguinte ambiente foi configurado:
* **Sistema Operacional:** Windows com subsistema WSL 2 rodando Ubuntu Linux.
* **Ferramentas:** Zephyr SDK v0.17.4, emulador QEMU, CMake, utilitário `west`e Python 3.12.
* **Linguagem:** C/C++.


## 3. Implementação e Arquitetura


### 3.1 Requisito Principal
A implementação da tarefa principal foi realizada limpando e reescrevendo o arquivo `main.c` de um projeto base (https://github.com/zephyrproject-rtos/example-application). Para cumprir a exigência do uso de bibliotecas nativas, utilizou-se a macro `#include <zephyr/kernel.h>` para acesso às funções de tempo e *threads*, e `#include <zephyr/sys/printk.h>`para a impressão no console. O uso da função `printk()`foi priorizado em relação ao `printf()` tradicional do C, por ser mais leve e otimizado para o kernel do RTOS.


### 3.2 Feature Adicional (Bônus)
Como *feature* adicional, foi desenvolvida uma arquitetura produtor-consumidor utilizando Filas de Mensagens (*Message Queues*):
* **Thread Produtor (`sensor_thread`):** Simula a variação da pressão da água em uma tubulação (subindo e descendo os valores automaticamente) e os envia para a fila.
* **Thread Consumidor (`alerta_thread`):** Aguarda a chegada das mensagens e implementa a lógica de decisão. Foi implementada uma Máquina de Estados Finita (*FSM - Finite State MAchine*) para deixar de usar if/else, evitando que o código fique confuso caso o sistema cresça.
* **Controle de Estados:** Foi criado um `enum` para que o sistema passe a ter "memória" do que está acontecendo, entendendo se já estava em um estado ou se é um novo. O uso do `enum` facilita a leitura humana e a compilação do processador, que substitui as palavras por números automaticamente, sendo mais veloz e eficiente. O `typedef` declara a variável de froma direta, utilizando a conveção `_t`.
* **Transições Seguras:** Utilizando um `switch/case`, o sistema possui transições claras: ele sabe quando a pressão subiu ou desceu e não fica disparando alarmes repetidas vezes, mas avisa que a eergência continua, e qaundo volta ao normal também notifica.
* **Estrutura de Dados:** Os dados trafegam na fila envelopados em uma `struct`. Essa abordagem foi escolhida para garantir organização, clareza e alta escalabilidade. Caso seja necessário adicionar a leitura de novos sensores no futuro, basta incluir novas variáveis na `struct`e o restante do código calculara os bytes automaticamnete, adaptando-se sem quebrar a lógica existente.


### 3.3 Boas Práticas e Organização
Tendo em vista que o código atuará como um produto mantido por uma equipe de desenvolvedores com diferentes níveis, o projeto foi estruturado com documentação em formato de comentários diretos no código-fonte.
Como as *threads* definidas com `K_THREAD_DEFINE` iniciam sozinhas durante o boot do sistema, o código foi modularizado, a lógica da tubulação foi separada em dois arquivos: `tubulacao.h`e `tubulacao.c`. Dessa forma a main.c fica mais limpa, contendo apenas o desafio obrigatório. O sistema de compilação foi devidamente avisado sobre a criação do novo arquivo ajustamdo a linha `target_sources` no `CMkaeLists.txt`.
Também foi utilizado o macro oficial `K_MSGQ_DEFINE` garantindo uma alocação segura de memória e alinhamento de leitura de 4 bytes, maximixando a performance do processador no emulador.


## 4. Dificuldades Encontradas e Soluções


Durante a execução do projeto, alguns desafios de configuração e sintaxe foram superados:
* **Incompatibilidade da versão do Python:** O ambiente Ubunto possuía o Python 3.10.12, porém a compilação do Zephyr exigia no mínimo a versão 3.12, gerando falha no CMake.
    * *Solução:* Adição do repositório *deadsnakes*, instalação do Python 3.12 e atualização das alternativas do sistema para defini-lo como pasrão, reinstalando os pacotes do `west`via `pip`logo em seguida.
* **Erro de configuração do kconfig (variável BLINK):** Ao limpar a `main.c` do projeto de exemplo para construir a nova solução, a compilação foi abortada pelo erro `attempt to assign the value 'y' to the undefined symbol BLINK`.
    * *Solução:* Foi identificado que o arquivo `prj.conf` ainda ordenava a ativação de um LED (`CONFIG_BLINK=y`), que não existia mais no código. O arquivo foi limpo, mantendo apenas a diretiva `CONFIG_PRINTK=y`, resolvendo a restrição.
* **Ajuste de Sintaxe e Tempo:** Pequenos erros de compilação, como a falta de `;` na macro da Fila de Mensagens e a velocidade excessiva dos *logs* no console.
 * *Solução:* Correção sintática da inicialização da `struct`e inclusão da função nativa `k_msleep()` para cadenciar a simulação de forma visível e realista no terminal.
* **Comportamento inesperado de temporização no emulador:** Durante a execução das *threads* de telemetria, foi observado que a simulação das leituras não respeitou oo tempo de pausa estipulado pela função `k_msleep()`, rodando o log no console em velocidade acelerada, dificultando a leitura humana.
    * *Solução e Decisão de Projeto:* A primeira suspeita foi de um possível *Stack Overflow*, então o primeiro passo foi aumentar o tamanho da pilha das *threads* de 1024 para 4096 bytes. Como os logs continuavam acelerados, conclui que era uma instabilidade no relógio de sistema do hardware virtualizado `qemu_86`rodando via WSL2, após pesquisar foi encontrado que rodar o QEMU no WSL (especialmente no WSL2) poderia causar problemas relacionados a `Systick`, contagem de tempo e interrupções temporalizada. Opstou-se por manter a implemnetação, pois o código cumpre os requisitos de arquitetura, uso nativo de *Message Queues* e *Threads* do Zephyr. como solução prática para avaliação, a thread produtor foi limitada a um laço `for` de iterações limitadas (mantendo o `while(1)`padrão de RTOS comentado e documentado no código-fonte), garantindo que o avaliador consiga visualizar os alertas no console.


## 5. Instrução de Execução

### Pré-requisitos
Para compilar e executar este projeto localmente utilizando o emulador QEMU, é estritamente necessário ter o ambiente do Zephyr (SDK e ferramentas atreladas) devidamente configurado. Caso o ambiente não esteja pronto, consulte a documentação oficial:
* [Guia de Instalação do Zephyr (Getting Started)](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
* [Instalação do Zephyr SDK](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html)

### Passo a Passo
Após garantir a configuração do ecossistema Zephyr, siga os passos abaixo no terminal:

1. Clone o repositório da aplicação:
```Bash
    git clone https://github.com/pamela-p/desafio-raptor-zephyr.git
    cd desafio-raptor-zephyr
```


2. Compile o projeto voltado para arquitetura x86 simulada:
```Bash
    west build -b qemu_x86 app
```


3. Execute no emulador QEMU (terminal aberto an pasta):
```Bash
    west build -t run
```
(Para encerrar a simulação e retorna ao terminal, pressione `Ctrl + A`, solte e pressione a tecla `X`).


## 6. Conclusão
O Projeto atendeu com sucesso aos requisitos estipulados, comprovando funcionamento do ZephyrOS em ambiente virtualizado. A adição das Message Queues validou conceitos de sistema embarados e RTOS.
