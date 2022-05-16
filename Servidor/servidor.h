#pragma once
#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHAVE_REGISTRY_NOME TEXT("SOFTWARE\\SO2_PIPE_DREAM")
#define REGISTRY_TAM_H TEXT("TAM_HORIZONTAL")
#define REGISTRY_TAM_V TEXT("TAM_VERTICAL")
#define REGISTRY_TEMPO TEXT("TEMPO_AGUA")
#define TAM_H_OMISSAO 10
#define TAM_V_OMISSAO 7
#define TEMPO_AGUA_OMISSAO 10L
#define EVENT_FLUIR_AGUA TEXT("EVENT_FLUIR_AGUA")
#define TIMER_FLUIR 2

#define MAX 256
#define N_STR 5
#define TAM_BUFFER 10
#define NUM_SV 1

#define MEM_PARTILHADA _T("MEM_PARTILHADA")			//nome da mem partilhada
#define MUTEX_BUFFER _T("MUTEX_BUFFER")				//nome da mutex do buffer circular
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")							//nome do semaforo de servidor

#define EVENT_TERMINAR _T("EVENT_TERMINAR")
#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")
#define MUTEX_TABULEIRO _T("MUTEX_TABULEIRO")

#define CIMA 0
#define DIREITA 1
#define BAIXO 2
#define ESQUERDA 3


/*comandos*/
#define PFAGUA _T("PFAGUA")							//nome comando para fluxo agua durante um periodo de tempo
#define BARR _T("BARR")								//nome comando adiciona barreira á grelha de jogo		
#define MODO _T("MODO")								//nome comando modo sequencia de peças/tubos
#define SAIR _T("SAIR")								//sair
#define INICIAR _T("INICIAR")						//iniciar jogo
#define PAUSA _T("PAUSA")							//pausar jogo
#define RETOMAR _T("RETOMAR")						//retomar jogo


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

typedef struct {
	int(*tabuleiro)[20][20];						//ponteiro para o tabuleiro referente na memória partilhada // TER CUIDADO QUANDO SE DESMAPEAR A MEMÓRIA VIRTUAL
	int posX, posY;									//posição da água
	unsigned int dirAgua;							//direção da água	// 0 > cima , 1 > direita, 2 > baixo, 3 > esquerda
}DadosTabuleiro;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hSemServidor;							//semaforo para controlo do número de svs ativos
	HANDLE hMutexBufferCircular;					//mutex buffer circular
	HANDLE hMapFile;								//map file
	HANDLE hThreadAgua;								//handle thread agua
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
	DWORD tempoInicioAgua;							//tempo até água começar a fluir
	DadosTabuleiro tabuleiro1, tabuleiro2;
	int parafluxo;									//para thread fluxo agua por determinado tempo

	HANDLE hEventUpdateTabuleiro;					//evento que indica aos monitores que houve alterações nos tabuleiros
	HANDLE hMutexTabuleiro;
	BOOL iniciado;
	HANDLE hEventTerminar;							//evento para informar monitores (e clientes na meta 2) que devem terminar
	int posfX, posfY;								//posição peça final da água

}DadosThread;

TCHAR** divideString(TCHAR* comando, const TCHAR* delim, unsigned int* tam);

DWORD WINAPI ThreadConsumidor(LPVOID param);

void carregaMapaPreDefinido(DadosThread* dados, int tabuleiro[20][20]);

BOOL definirInicioFim(DadosThread* dados);

BOOL fluirAgua(DadosThread* dados);

DWORD WINAPI ThreadAgua(LPVOID param);

BOOL initMemAndSync(DadosThread* dados, unsigned int tamH, unsigned int tamV);

DWORD carregaValorConfig(TCHAR valorString[], HKEY hChaveRegistry, TCHAR nomeValorRegistry[], unsigned int valorOmissao, unsigned int* varGuardar, unsigned int min, unsigned int max);

#endif