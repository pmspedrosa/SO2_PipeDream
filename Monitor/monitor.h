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
#define N_STR 5
#define TAM_BUFFER 10
#define NUM_SV 1

#define MEM_PARTILHADA _T("MEM_PARTILHADA")			//nome da mem partilhada
#define MUTEX_BUFFER _T("MUTEX_BUFFER")				//nome da mutex
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")							//nome do semaforo de servidor

#define EVENT_TERMINAR _T("EVENT_TERMINAR")
#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")
#define BLUE _T("\x1b[34m")
#define RESET _T("\x1b[0m")
#define MUTEX_TABULEIRO _T("MUTEX_TABULEIRO")

/*comandos*/
#define PFAGUA _T("PFAGUA")							//nome do semaforo de leitura
#define BARR _T("BARR")								//nome do semaforo de leitura
#define SAIR _T("SAIR")

typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	TCHAR comando[MAX];								//comando produzido
}CelulaBuffer;

typedef struct {
	unsigned int nP;								//numero de produtores		//acho que isto não tem de estar em memória partilhada
	unsigned int posE;								//posicao de escrita
	unsigned int posL;								//posicao de leitura
	int tabuleiro1[20][20];							//tabuleiro jogador 1
	int tabuleiro2[20][20];							//tabuleiro jogador 2
	unsigned int tamX, tamY;						//tam tabuleiro
	CelulaBuffer buffer[TAM_BUFFER];
	TCHAR estado[MAX];								//string que indica o estado do programa
}MemPartilhada;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hSemServidor;							//semaforo para controlo do número de svs ativos
	HANDLE hMutexBufferCircular;					//mutex buffer circular
	HANDLE hMapFile;								//map file
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
	HANDLE hEventTerminar;							//evento assinalado pelo servidor para sair
}DadosThread;


DWORD WINAPI ThreadProdutor(LPVOID param);

VOID displayTabuleiro(int tab[20][20], int tamX, int tamY, HANDLE hConsole, WORD atributosBase);

VOID updateDisplay(DadosThread* dados, HANDLE hConsole, WORD atributosBase);

DWORD WINAPI ThreadDisplay(LPVOID params);

BOOL initMemAndSync(DadosThread* dados);

#endif