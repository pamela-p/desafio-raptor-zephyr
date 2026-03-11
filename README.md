# Relatório Técnico: Desafio Sistemas Embarcados (Raptor)


**Autora:** Pâmela da Silva Paes
**Data:** Março/2026


-------------------------------------------------------------------------


## 1. Introdução
Este projeto tem como objetivo elaborar uma solução em ambiente de simulação usando QEMU para o Sistema Operacional de Tempo Real (RTOS) ZephyrOS. O escopo principal consiste em exibir a mensagem "Hello World! This is Raptor!" em ambiente de virtualização. Além do requisito obrigatório, foi desenvolvida uma *feature* adicional utilizando *Message Queues* e uma *Finite State Machine*(FSM) para simular a telemetria, o monitoramento e a lógica estruturada de alertas e o **acionamento de hardware (GPIO)** baseada na pressão de água de um reservatório.


## 2. Ambiente de Desenvolvimento
Para o desenvolvimento e simulação, o seguinte ambiente foi configurado:
* **Sistema Operacional:** Windows com subsistema WSL 2 rodando Ubuntu Linux.
* **Ferramentas:** Zephyr SDK v0.17.4, emulador QEMU, CMake, utilitário `west`e Python 3.12.
* **Linguagem:** C/C++.


## 3. Implementação e Arquitetura


### 3.1 Requisito Principal
A implementação da tarefa principal foi realizada limpando e reescrevendo o arquivo `main.c` de um projeto base (https://github.com/zephyrproject-rtos/example-application). Para cumprir a exigência do uso de bibliotecas nativas, utilizou-se a macro `#include <zephyr/kernel.h>` para acesso às funções de tempo e *threads*, e `#include <zephyr/sys/printk.h>`para a impressão no console. O uso da função `printk()`foi priorizado em relação ao `printf()` tradicional do C, por ser mais leve e otimizado para o kernel do RTOS.


### 3.2 Feature Adicional (Bônus)
Como *feature* adicional, foi desenvolvida uma arquitetura avançada de produtor-consumidor operando como um **Sistema de Controle de Malha Fechada (*Closed-Loop*)**, utilizando Filas de Mensagens e Variáveis Atômicas:
* **Thread Produtora (`sensor_thread`):** Simula o sensor de pressão do reservatório. Diferente de uma simulação estática, este sensor reage ativamente aos comandos do sistema, subindo ou descendo a pressão conforme o estado da válvula virtual.
* **Thread Consumidora (`alerta_thread`):** Aguarda a chegada das mensagens e implementa a lógica de decisão através de uma Máquina de Estados Finita (*FSM - Finite State Machine*), evitando a complexidade de múltiplos blocos `if/else`.
* **Malha de Controle com Histerese:** A FSM atua no sistema simulando uma bomba/válvula de entrada com uma janela de tolerância (histerese). A bomba liga para encher o tanque (pressão sobe) e desliga ao atingir o limite de segurança superior (130 PSI), permitindo que a pressão caia naturalmente simulando o consumo da água, até atingir o limite inferior (70 PSI), reiniciando o ciclo de forma autônoma.
* **Sincronização Segura com Variáveis Atômicas:** Foi utilizada a biblioteca `<zephyr/sys/atomic.h>` para garantir que as *threads* conversem em total segurança estrutural bidirecional (Fila de Mensagens em um sentido, Variável Atômica no outro). Isso previne cenários de Condição de Corrida (*Race Condition*), pois impede que uma *thread* tente ler a posição da válvula no exato momento em que a outra a escreve, garantindo operações indivisíveis pelo processador.
* **Integração de Hardware (GPIO Virtual):** Como o desenvolvimento ocorreu em um emulador (`qemu_x86`) sem a presença de uma placa física (como a família STM32), optou-se por abstrair o hardware através da *Devicetree*. Foi criado um arquivo `app.overlay` para injetar um pino de saída virtual no sistema. Através da biblioteca `<zephyr/drivers/gpio.h>`, a FSM envia comandos elétricos reais para esse pino virtual, simulando o acionamento físico de uma válvula solenoide ou relé.
* **Controle de Estados:** Foi criado um `enum` para que a FSM tenha "memória" do evento atual. O uso do `enum` facilita a leitura humana e otimiza a compilação, já que o processador substitui as palavras por números automaticamente. O `typedef` declara a variável de forma direta com a convenção `_t`.
* **Transições Seguras:** Utilizando um `switch/case`, o sistema possui transições claras: aciona mecanismos apenas na mudança de estado e emite alertas contínuos sem redundância de comandos de hardware.
* **Estrutura de Dados:** Os dados trafegam na fila envelopados em uma `struct`, garantindo organização, clareza e alta escalabilidade para a adição de novos sensores no futuro.


### 3.3 Boas Práticas e Organização
Tendo em vista que o código atuará como um produto mantido por uma equipe de desenvolvedores com diferentes níveis, o projeto foi estruturado com documentação em formato de comentários diretos no código-fonte.
Como as *threads* definidas com `K_THREAD_DEFINE` iniciam sozinhas durante o boot do sistema, o código foi modularizado, a lógica da tubulação foi separada em dois arquivos: `tubulacao.h`e `tubulacao.c`. Dessa forma a main.c fica mais limpa, contendo apenas o desafio obrigatório. O sistema de compilação foi devidamente avisado sobre a criação do novo arquivo ajustando a linha `target_sources` no `CMakeLists.txt`.
Também foi utilizado o macro oficial `K_MSGQ_DEFINE` garantindo uma alocação segura de memória e alinhamento de leitura de 4 bytes, maximizando a performance do processador no emulador.


## 4. Dificuldades Encontradas e Soluções


Durante a execução do projeto, alguns desafios de configuração e sintaxe foram superados:
* **Incompatibilidade da versão do Python:** O ambiente Ubuntu possuía o Python 3.10.12, porém a compilação do Zephyr exigia no mínimo a versão 3.12, gerando falha no CMake.
    * *Solução:* Adição do repositório *deadsnakes*, instalação do Python 3.12 e atualização das alternativas do sistema para defini-lo como padrão, reinstalando os pacotes do `west`via `pip`logo em seguida.
* **Erro de configuração do kconfig (variável BLINK):** Ao limpar a `main.c` do projeto de exemplo para construir a nova solução, a compilação foi abortada pelo erro `attempt to assign the value 'y' to the undefined symbol BLINK`.
    * *Solução:* Foi identificado que o arquivo `prj.conf` ainda ordenava a ativação de um LED (`CONFIG_BLINK=y`), que não existia mais no código. O arquivo foi limpo, mantendo apenas a diretiva `CONFIG_PRINTK=y`, resolvendo a restrição.
* **Ajuste de Sintaxe e Tempo:** Pequenos erros de compilação, como a falta de `;` na macro da Fila de Mensagens e a velocidade excessiva dos *logs* no console.
 * *Solução:* Correção sintática da inicialização da `struct`e inclusão da função nativa `k_msleep()` para cadenciar a simulação de forma visível e realista no terminal.
* **Comportamento inesperado de temporização no emulador:** Durante a execução das *threads* de telemetria, foi observado que a simulação das leituras não respeitou o tempo de pausa estipulado pela função `k_msleep()`, rodando o log no console em velocidade acelerada, dificultando a leitura humana.
    * *Solução e Decisão de Projeto:* A primeira suspeita foi de um possível *Stack Overflow*, então o primeiro passo foi aumentar o tamanho da pilha das *threads* de 1024 para 4096 bytes. Como os logs continuavam acelerados, concluí que era uma instabilidade no relógio de sistema do hardware virtualizado `qemu_x86`rodando via WSL2, após pesquisar foi encontrado que rodar o QEMU no WSL (especialmente no WSL2) poderia causar problemas relacionados a `Systick`, contagem de tempo e interrupções temporalizada. Optou-se por manter a implementação, pois o código cumpre os requisitos de arquitetura, uso nativo de *Message Queues* e *Threads* do Zephyr. como solução prática para avaliação, a thread produtor foi limitada a um laço `for` de iterações limitadas (mantendo o `while(1)`padrão de RTOS comentado e documentado no código-fonte), garantindo que o avaliador consiga visualizar os alertas no console.
* **Ausência de Hardware Nativo no Emulador (Erro GPIO):** Ao tentar implementar a abstração de hardware com GPIO na Devicetree, a compilação falhou com o erro `undefined node label 'gpio0'`.
    * *Solução:* Foi concluido que o `qemu_x86` simula a placa-mãe de um PC, que não possui pinos GPIO nativos. Para resolver, foi fabricado um controlador de pinos virtual (emulado) no arquivo `app.overlay` através do nó `compatible = "zephyr,gpio-emul"`. Em seguida, as diretivas `CONFIG_EMUL=y` e `CONFIG_GPIO_EMUL=y` foram adicionadas ao `prj.conf` para alertar o sistema de compilação sobre o uso de drivers virtuais.


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
    west build -b qemu_x86 app -p
```


3. Execute no emulador QEMU (terminal aberto na pasta):
```Bash
    west build -t run
```
(Para encerrar a simulação e retornar ao terminal, pressione `Ctrl + A`, solte e pressione a tecla `X`).


## 6. Conclusão
O Projeto atendeu com sucesso aos requisitos estipulados, comprovando funcionamento do ZephyrOS em ambiente virtualizado. A adição das Message Queues, Máquina de Estados e integração GPIO validou conceitos avançados de sistemas embarcados e abstração de hardware em RTOS.

## 7. Trabalhos Futuros
Como a lógica de controle de malha fechada e a abstração de hardware via GPIO já foram implementadas e validadas no emulador, os próximos passos para escalonar a solução incluem:

* **Deploy em Hardware Físico (STM32):** O código atual está pronto para ser portado para microcontroladores reais. O próximo passo é compilar a solução para uma placa física (como a família STM32), bastando alterar o target no momento do build e mapear o relé da válvula solenoide para o pino correspondente na Devicetree física.
* **Integração IoT:** Expansão da thread consumidora para enviar os logs de telemetria lidos na Fila de Mensagens para um dashboard em nuvem ou servidor local.