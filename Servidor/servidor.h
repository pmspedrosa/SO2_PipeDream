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
#include "utils.h"

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
#define MUTEX_NPIPE_SV _T("MUTEX_NPIPE_SV")						//nome mutex named Pipe servidor
#define MUTEX_NPIPE_CLI _T("MUTEX_NPIPE_CLI")					//nome mutex named Pipe cliente
/*semaforos*/
#define SEM_ESCRITA _T("SEM_ESCRITA")							//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")							//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")										//nome do semaforo de servidor
/*eventos*/
#define EVENT_TERMINAR _T("EVENT_TERMINAR")						//nome eveto terminar
#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")					//nome evento tabuleiro
#define EVENT_NAMEDPIPE_SV _T("EVENT_NAMEDPIPE_SV")				//nome evento named pipe servidor
#define EVENT_NAMEDPIPE_CLI _T("EVENT_NAMEDPIPE_CLI")			//nome evento named pipe cliente
#define EVENT_OVERLAP _T("EVENT_OVERLAP")						//nome evento overlapped


#define TAM_H_OMISSAO 10										//tamanho horizontal default
#define TAM_V_OMISSAO 7											//tamanho vertical default
#define TEMPO_AGUA_OMISSAO 10L									//tempo inicio jogo default - 10 segundos
#define TIMER_FLUIR 3											//tempo fluir �gua
#define AUMENTO_VELOCIDADE 0.8f
#define NUM_PARAGENS 3											//numero de pausas que um jogador pode fazer (hover)
#define MAX 256									
#define TAM_BUFFER 10											//tamanho buffer
#define NUM_SV 1												//numero de servidor ativos possiveis
#define NPIPES 2												//Numero de pipes

/*dire��es*/
#define CIMA 0
#define DIREITA 1
#define BAIXO 2
#define ESQUERDA 3

/*comandos_msgs*/
#define PFAGUA _T("PFAGUA")										//nome comando para fluxo agua durante um periodo de tempo
#define BARR _T("BARR")											//nome comando adiciona barreira � grelha de jogo		
#define MODO _T("MODO")											//nome comando modo sequencia de pe�as/tubos
#define SAIR _T("SAIR")											//sair
#define INICIAR _T("INICIAR")									//iniciar jogo
#define PAUSA _T("PAUSA")										//pausar jogo
#define RETOMAR _T("RETOMAR")									//retomar jogo
#define PONTUACAO _T("PONTUACAO")								//mostrar pontuacoes

#define LCLICK _T("LCLICK")
#define RCLICK _T("RCLICK")
#define HOVER _T("HOVER")
#define RETOMAHOVER _T("RETOMAHOVER")
#define SAIRCLI _T("SAIRCLI")
#define JOGOSINGLEP _T("JOGOSINGLEP")
#define JOGOMULTIP _T("JOGOMULTIP")
#define JOGOMULTIPCANCEL _T("JOGOMULTIPCANCEL")
#define INICIAJOGO _T("INICIAJOGO")									//Inicia Jogo
#define INICIAJOGOMP _T("INICIAJOGOMP")								//Inicia Jogo Multiplayer
#define PROXNIVEL _T("PROXNIVEL")


#define NOME_PIPE_CLIENTE _T("\\\\.\\pipe\\cliente")					//nome named pipe Cliente-Servidor
#define NOME_PIPE_SERVIDOR _T("\\\\.\\pipe\\servidor")				//nome named pipe Servidor-Cliente



typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	TCHAR comando[MAX];								//comando produzido
}CelulaBuffer;

typedef struct {
	BOOL jogadorAtivo;								//indica se esta estrutura est� a ser utilizada
	int tabuleiro[20][20];							//tabuleiro
}MemPartilhadaTabuleiro;

typedef struct {
	unsigned int nP;								//numero de produtores
	unsigned int posE;								//posicao de escrita
	unsigned int posL;								//posicao de leitura
	MemPartilhadaTabuleiro dadosTabuleiro1;			//dados do tabuleiro 1
	MemPartilhadaTabuleiro dadosTabuleiro2;			//dados do tabuleiro 2
	unsigned int tamX, tamY;						//tam tabuleiro
	CelulaBuffer buffer[TAM_BUFFER];				//array buffer circular de estruturas CelulaBuffer
	TCHAR estado[MAX];								//string que indica o estado do programa
}MemPartilhada;


typedef struct {
	OVERLAPPED overlap;								//OVERLAPPED para leitura por parte do servidor
	HANDLE hPipeOut;								//pipe para comunica��o // servidor -> cliente
	HANDLE hPipeIn;									//pipe para comunica��o // cliente -> servidor
}InfoPipesTabuleiro;

typedef struct {
	TCHAR nome[MAX];								//nome do jogador
	int vitorias;									//n�mero de vit�rias (indica o n�mero de n�veis passados)
}DadosPontuacao;

typedef struct {
	BOOL* jogadorAtivo;								//indica se est� estrutura esta a ser utilizada
	HANDLE hThreadAgua;								//handle thread de fluxo de �gua
	int(*tabuleiro)[20][20];						//ponteiro para o tabuleiro referente na mem�ria partilhada
	int posX, posY;									//posi��o da �gua
	unsigned int dirAgua;							//dire��o da �gua	// 0 > cima , 1 > direita, 2 > baixo, 3 > esquerda
	int sequencia[6];								//sequencia de tubos
	InfoPipesTabuleiro pipes;						//pipes utilizados por este tabuleiro
	int numParagensDisponiveis;						//n�mero de pausas que o jogador pode fazer
	BOOL correrAgua;								//controlador da thread de fluxo de �gua
	DadosPontuacao pontuacao;						//dados sobre pontua��o do jogador
}DadosTabuleiro;

typedef struct {									//estrutura de dados principal
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
	int posfX, posfY;								//posi��o pe�a final da �gua
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
	int parafluxo;									//para thread fluxo agua por determinado tempo
	DWORD tempoInicioAgua;							//tempo at� �gua come�ar a fluir
	float velocidadeAgua;							//multiplicador da velocidade da agua, por n�vel
	BOOL iniciado;									//True -  jogo foi iniciado, False - n�o
	BOOL modoRandom;								//TRUE -> modo de sequencia random //FALSE -> modo de sequencia definida

	HANDLE hMutexNamedPipe;							//mutex sobre a mensagem a enviar pelo pipe
	HANDLE hEventoNamedPipe;						//evento que aciona a escrita nos pipes
	HANDLE hThreadLer;								//thread de leitura de pipes
	HANDLE hThreadEscrever;							//thread de escrita nos pipes
	TCHAR mensagem[MAX];							//mensagem a ser escrita nos pipes
	HANDLE hPipeOut;								//pipe em que a thread de escrita escreve
	int multi;										//quantidade de jogadores no modo multiplayer
}DadosThread;

typedef struct {
	DadosThread* dados;
	DadosTabuleiro* dadosTabuleiro;
}DadosThreadAgua;

TCHAR** divideString(TCHAR* comando, const TCHAR* delim, unsigned int* tam);
void prepararInicioDeJogo(DadosThread* dados);
void alteraSequencia(DadosThread* dados, DadosTabuleiro* tabuleiro);
DadosTabuleiro* leDePipesOverlapped(DadosThread* dados, int* n, TCHAR buf[MAX]);
DWORD WINAPI ThreadLer(LPVOID param);
BOOL escreveNamedPipe(DadosThread* dados, TCHAR* a, DadosTabuleiro* dadosTabuleiro);
BOOL verificaColocarPeca(DadosThread* dados, int x, int y, int t, DadosTabuleiro* sourceTabuleiro);
DWORD WINAPI ThreadEscrever(LPVOID param);
DWORD WINAPI ThreadConsumidor(LPVOID param);
void carregaMapaPreDefinido(DadosThread* dados, int tabuleiro[20][20]);
BOOL definirInicioFim(DadosThread* dados);
BOOL fluirAgua(DadosThread* dados, DadosTabuleiro* dadosTabuleiro);
DWORD WINAPI ThreadAgua(LPVOID param);
BOOL initMemAndSync(DadosThread* dados, unsigned int tamH, unsigned int tamV);
DWORD carregaValorConfig(TCHAR valorString[], HKEY hChaveRegistry, TCHAR nomeValorRegistry[], unsigned int valorOmissao, unsigned int* varGuardar, unsigned int min, unsigned int max);
BOOL initNamedPipes(DadosThread* dados);
int terminar(DadosThread* dados);

#endif