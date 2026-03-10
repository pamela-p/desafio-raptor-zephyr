/*******************************************************************************
 * tubulacao.h
 * Definição das estruturas de dados utilizadas para a telemetria
 * e monitoramento da tubulaçao do reservatóio.
*******************************************************************************/

#ifndef TUBULACAO_H
#define TUBULACAO_H

/*******************************************************************************
 * Estrutura de dados da fila (Payload)
 * Struct para guardar o pacote de dados (mensagem) que trafegará através da Message Queue
*******************************************************************************/
struct sensor_data_type{
	int pressao_da_agua;
};

#endif /* TUBULACAO_H */