/*******************************************************************************
 * tubulacao.c
 * Implementação das threads de monitoramento, telemetria e controle de pressão
*******************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "tubulacao.h" // Importando a definição das estruturas

/* Inicializa estaticamente uma Message Queue no Zephyr
 * Parâmetros da macro K_MSGQ_DEFINE:
 * 1. Nome da fila: sensor_msgq
 * 2. Tamanho de cada mensagem: sizeof(struct sensor_data_type)
 * 3. Capacidade máxima: 13 mensagens na fila
 * 4. Alinhamnto de memória: 4 bytes (QEMU lê a memoria de 4 em 4 bytes, dessa forma leitura rápida é otimizada)
*/
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data_type), 13, 4);

/*******************************************************************************
 * Máquina de Estados Finita (FSM)
 * Enumeração dos estados do reservatório
*******************************************************************************/
typedef enum{
	ESTADO_NORMAL,
	ESTADO_PRESSAO_ALTA,
	ESTADO_PRESSAO_BAIXA
} estado_reservatorio_t;


/*******************************************************************************
 * THREAD 1: Produtor (simulador do sensor de pressão)
 * * Em Zephyr, a assinatura padrão de uma thread exige três ponteiros genéricos
 * (void *). Como não há passagem de parâmetros externos neste escopo,
 * os argumentos serão recebidos como NULL durante a inicialização
*******************************************************************************/
void sensor_thread(void *p1, void *p2,void *p3){
	struct sensor_data_type data;
	int pressao_atual = 100; // pressão de inicialização
	int variacao = 15; // variação para subir e descer a cada leitura
	
    // Pausa inicial para garantr a exibição prévia dos logs de boot da main
    k_msleep(2000);

    /* NOTA PARA AVALIAÇÃO:
     * Em hardware real, o monitoramento rodaria em loop inifnito (para isso descomentar o while abaixo)
     * Contudo, devido a limitação de sincronismo do emulador QEMU rodando via WSL, que não esta respeitando o k_msleep corretamente,
	 * a execução foi intencionalemnte limitada a 30 iterações (laço for) para garantir que
	 * oslogs de telemetria possam ser lidos com clareza no console
	*/

	// while(1){
    for (int i=0; i < 30; i++){
		// simulação da variação
		pressao_atual += variacao;

 
		// Inverte o sinal da variação ao atingir limites críticos
		if (pressao_atual > 150 || pressao_atual < 50){
			variacao = -variacao;
		} 

		// Encapsulamento dos dados
		data.pressao_da_agua = pressao_atual;

		/*
		 * Transmite o dado para a Fila de Mensagens. 
		 * Utiliza-se a política K_NO_WAIT (timeout zero) para garantir que a
		 * thread produtora não sofra bloqueio caso a fila esteja cheia.
		*/
		k_msgq_put(&sensor_msgq, &data, K_NO_WAIT);

        // Cadência desimulação 
        k_msleep(3000);
	}

    printk("\n[SISTEMA] Fim da simulacao de leitura (Limite de 30 atingido).\n");
    // se descomentado o while, comentar for e esse printk
}

/*******************************************************************************
 * THREAD 2: Consumidor (lê a fila e opera a FSM)
*******************************************************************************/
void alerta_thread(void *p1, void *p2, void *p3){
	struct sensor_data_type data_recebida;

	// Inicialização segura da FSM
	estado_reservatorio_t estado_atual = ESTADO_NORMAL;

	printk("\n Iniciando monitoramento de pressao via FSM\n\n");

	while(1){
		/*
		 * Bloqueia a thread atual e aguarda a chegada de novas mesndagens na fila
		 * Foi utilizado K_FOREVER para evitar consumo desnecessário de CPU
		 * enquanto não houver dados disponíveis 
		*/
		k_msgq_get(&sensor_msgq, &data_recebida, K_FOREVER);

		int pressao = data_recebida.pressao_da_agua;
		printk("Leitura do sensor: %d PSI ", pressao);
		
		/**
		 * A Máqquina de Estados avalia as transições com base no estado atual
		 * do reservatório, garantindo que as ações corretivas (como acionar a válvula)
		 * ocorram apenas nas muanças de estado, evitando a emissão de comandos redundates.
		*/
		switch(estado_atual){
			case ESTADO_NORMAL:
				if (pressao >= 130){
					estado_atual = ESTADO_PRESSAO_ALTA;
					printk("MUDANCA: Normal -> ALTO! Acionar valvula.\n\n");
				}else if(pressao <= 70){
					estado_atual = ESTADO_PRESSAO_BAIXA;
					printk("MUDANCA: Normal -> BAIXO! Vericar vazamento.\n\n");
				}else{
					printk("Status: Normal.\n\n");
				}
				break;
			
			case ESTADO_PRESSAO_ALTA:
				if (pressao < 130){
					estado_atual = ESTADO_NORMAL;
					printk("MUDANCA: Alto -> NORMAL! Fechar valvula.\n\n");
				}else {
					printk("Status: Pressao ALTA continua.\n\n");
				}
				break;
			
			case ESTADO_PRESSAO_BAIXA:
				if (pressao > 70){
					estado_atual = ESTADO_NORMAL;
					printk("MUDANCA: Baixo -> NORMAL! Fluxo estabilizado.\n\n");
				}else {
					printk("Status: Pressao BAIXA continua.\n\n");
				}
				break;
		}

	}
}

/*******************************************************************************
 * Inicialização e Registro no Kernel
 * K_THREAD_DEFINE: Define e inicializa as threads estaticamnete no boot
 * - Stack Size otimizado para 1024 bytes
 * - Nível de prioridade estabelecido em 7 (mediana)
*******************************************************************************/
K_THREAD_DEFINE(sensor_thread_name, 1024, sensor_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(alerta_thread_name, 1024, alerta_thread, NULL, NULL, NULL, 7, 0, 0);
