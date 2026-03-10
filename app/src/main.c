/*******************************************************************************
 * main.c
 * Arquivo principal de inicialização do sistema Raptor
******************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "tubulacao.h" // Importação da biblioteca de telemetria e alertas


/*******************************************************************************
 * Função Principal (Entry Point)
*******************************************************************************/
int main(void){
	/* REQUISITO OBRIGATÓRIO DO DESAFIO:
	 * Exibir uma mesagem de inicialização utilizando a API nativa do Zephyr
	 * Foi utilizado printk em substituição ao printf padrão do C por ser
	 * mais leve e otimizado para o kernel do RTOS	
	*/
	printk("\n\nHello World! This is Raptor!\n");

	/* Mantém a thread principal em suspensão (sleep) para não bloquear
	 * outras threads que estão trabalhando em paralelo
	 * As threads de telemetria da tubulação já foram iniciadas atutomaticamente
	 * durante o boot do sistema
	*/
	while(1){
		k_msleep(13000);
	}

	return 0;
}