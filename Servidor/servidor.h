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

/*registry*/
#define CHAVE_REGISTRY_NOME TEXT("SOFTWARE\\SO2_PIPE_DREAM")	//nome chave registry
#define REGISTRY_TAM_H TEXT("TAM_HORIZONTAL")					//nome dados tamanho horizontal registry
#define REGISTRY_TAM_V TEXT("TAM_VERTICAL")						//nome dados tamanho vertical registry
#define REGISTRY_TEMPO TEXT("TEMPO_AGUA")						//nome dados tempo antes de inicio fluxo de �gua registry
/*memoria partilhada*/
#define MEM_PARTILHADA _T("MEM_PARTILHADA")						//nome da mem partilhada
/*mutex*/
#define MUTEX_BUFFER _T("MUTEX_BUFFER")							//nome da mutex do buffer circular
#define MUTEX_TABULEIRO _T("MUTEX_TABULEIRO")					//nome mutex tabuleiro
/*semaforos*/
#define SEM_ESCRITA _T("SEM_ESCRITA")							//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")							//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")										//nome do semaforo de servidor
/*eventos*/
#define EVENT_TERMINAR _T("EVENT_TERMINAR")						//nome eveto terminar
#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")					//nome evento tabuleiro

#define TAM_H_OMISSAO 10										//tamanho horizontal default
#define TAM_V_OMISSAO 7											//tamanho vertical default
#define TEMPO_AGUA_OMISSAO 10L									//tempo inicio jogo default - 10 segundos
#define TIMER_FLUIR 2											//tempo fluir �gua
#define MAX 256									
#define TAM_BUFFER 10											//tamanho buffer
#define NUM_SV 1												//numero de servidor ativos possiveis

/*dire��es*/
#define CIMA 0
#define DIREITA 1
#define BAIXO 2
#define ESQUERDA 3

/*comandos*/
#define PFAGUA _T("PFAGUA")										//nome comando para fluxo agua durante um periodo de tempo
#define BARR _T("BARR")											//nome comando adiciona barreira � grelha de jogo		
#define MODO _T("MODO")											//nome comando modo sequencia de pe�as/tubos
#define SAIR _T("SAIR")											//sair
#define INICIAR _T("INICIAR")									//iniciar jogo
#define PAUSA _T("PAUSA")										//pausar jogo
#define RETOMAR _T("RETOMAR")									//retomar jogo


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

typedef struct {
	int(*tabuleiro)[20][20];						//ponteiro para o tabuleiro referente na mem�ria partilhada
	int posX, posY;									//posi��o da �gua
	unsigned int dirAgua;							//dire��o da �gua	// 0 > cima , 1 > direita, 2 > baixo, 3 > esquerda
}DadosTabuleiro;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;							//ponteiro para a memoria partilhada
	DadosTabuleiro tabuleiro1, tabuleiro2;			//guarda a estrutura onde se encontram os dados do tabuleiro
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hSemServidor;							//semaforo para controlo do n�mero de svs ativos
	HANDLE hMutexBufferCircular;					//mutex buffer circular
	HANDLE hMutexTabuleiro;							//mutex tabuleiro
	HANDLE hMapFile;								//map file
	HANDLE hEventUpdateTabuleiro;					//evento que indica aos monitores que houve altera��es nos tabuleiros
	HANDLE hEventTerminar;							//evento para informar monitores (e clientes na meta 2) que devem terminar
	HANDLE hThreadAgua;								//handle thread agua
	int posfX, posfY;								//posi��o pe�a final da �gua
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
	int parafluxo;									//para thread fluxo agua por determinado tempo
	DWORD tempoInicioAgua;							//tempo at� �gua come�ar a fluir
	BOOL iniciado;									//True -  jogo foi iniciado, False - n�o

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