#pragma once
#ifndef MONITOR_H
#define MONITOR_H

#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 256
#define TAM_BUFFER 10								//tamanho buffer		

#define MEM_PARTILHADA _T("MEM_PARTILHADA")			//nome da mem partilhada
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")							//nome do semaforo de servidor
/*eventos*/
#define EVENT_TERMINAR _T("EVENT_TERMINAR")			//evento terminar
#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")		//evento tabuleiro
/*mutex*/
#define MUTEX_TABULEIRO _T("MUTEX_TABULEIRO")		//mutex tabuleiro
#define MUTEX_BUFFER _T("MUTEX_BUFFER")				//nome da mutex

/*comandos*/
#define PFAGUA _T("PFAGUA")							//nome do semaforo de leitura
#define BARR _T("BARR")								//nome do semaforo de leitura
#define SAIR _T("SAIR")								//sair

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


DWORD WINAPI ThreadProdutor(LPVOID param);

VOID displayTabuleiro(int tab[20][20], int tamX, int tamY, HANDLE hConsole, WORD atributosBase);

VOID updateDisplay(DadosThread* dados, HANDLE hConsole, WORD atributosBase);

DWORD WINAPI ThreadDisplay(LPVOID params);

BOOL initMemAndSync(DadosThread* dados);

#endif