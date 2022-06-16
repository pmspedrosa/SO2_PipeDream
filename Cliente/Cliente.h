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


#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")					//evento tabuleiro
#define EVENT_NAMEDPIPE_SV _T("EVENT_NAMEDPIPE_SV")				//nome evento named pipe servidor
#define EVENT_NAMEDPIPE_CLI _T("EVENT_NAMEDPIPE_CLI")			//nome evento named pipe cliente

#define MUTEX_NPIPE_SV _T("MUTEX_NPIPE_SV")						//nome mutex named Pipe servidor
#define MUTEX_NPIPE_CLI _T("MUTEX_NPIPE_CLI")					//nome mutex named Pipe cliente



////////////////////////////////////////////////////////////////////////////////


#define NOME_PIPE_CLIENTE _T("\\\\.\\pipe\\cliente")					//nome named pipe Cliente-Servidor
#define NOME_PIPE_SERVIDOR _T("\\\\.\\pipe\\servidor")				//nome named pipe Servidor-Cliente


typedef struct {
	TCHAR cmd[MAX];									//comando da mensagem enviada. Ex: paraAgua, info, cliquedir
	TCHAR args[MAX][TAM_BUFFER];					//argumentos associados aos comandos, pode ter ou não
	int numargs;
}Msg;
/*
typedef struct {
	HANDLE hPipe;									// handle do pipe
	OVERLAPPED overlap;
	BOOL activo;									//representa se a instancia do named pipe está ou nao ativa, se ja tem um cliente ou nao
}PipeDados;*/

typedef struct {
	HANDLE hPipe;									// handle do pipe
	BOOL activo;									//representa se a instancia do named pipe está ou nao ativa, se ja tem um cliente ou nao
}PipeDados;
typedef struct {
	PipeDados hPipe;
	HANDLE hEventoNamedPipe;
	HANDLE hMutex;
	HANDLE hThread[2];
	int terminar;
}DadosThreadPipe;

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


#endif