#pragma once
#ifndef CLIENTE_H
#define CLIENTE_H

#include "resource.h"

#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 256
#define TAM_BUFFER 10								//tamanho buffer
#define NPIPES 2									//Numero de pipes		


#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")		//evento tabuleiro




////////////////////////////////////////////////////////////////////////////////


#define NOME_PIPE_CLIENTE _T("\\\.\\pipe\\cs")				//nome named pipe Cliente-Servidor
#define NOME_PIPE_SERVIDOR _T("\\\.\\pipe\\sc")				//nome named pipe Servidor-Cliente


typedef struct {
	TCHAR cmd[MAX];									//comando da mensagem enviada. Ex: paraAgua, info, cliquedir
	TCHAR args[MAX][TAM_BUFFER];					//argumentos associados aos comandos, pode ter ou não
}Msg;

typedef struct {
	HANDLE hPipe;									// handle do pipe
	OVERLAPPED overlap;
	BOOL activo;									//representa se a instancia do named pipe está ou nao ativa, se ja tem um cliente ou nao
}PipeDados;



////////////////////////////////////////////////////////////////////////////////



typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	TCHAR comando[MAX];								//comando produzido
}CelulaBuffer;

typedef struct {
	unsigned int nP;								//numero de produtores
	unsigned int posE;								//posicao de escrita
	unsigned int posL;								//posicao de leitura
	int tabuleiro1[20][20];							//tabuleiro jogador 1
	int tabuleiro2[20][20];							//tabuleiro jogador 2
	unsigned int tamX, tamY;						//tam tabuleiro
	CelulaBuffer buffer[TAM_BUFFER];				//array buffer circular de estruturas CelulaBuffer
	TCHAR estado[MAX];								//string que indica o estado do programa
}MemPartilhada;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;							//ponteiro para a memoria partilhada
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hMutexBufferCircular;					//mutex buffer circular
	HANDLE hMapFile;								//map file
	HANDLE hEventTerminar;							//evento assinalado pelo servidor para sair
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
}DadosThread;

typedef struct {
	PipeDados hPipe;
	HANDLE hEvent;
	HANDLE hMutex;
	int numClientes;
	int terminar;
}DadosThreadPipes;


#endif