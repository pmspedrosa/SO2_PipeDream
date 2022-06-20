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
#define ALTURA_INFO	20								//Altura da barra de informação
#define TAM_BITMAP 150
#define NUM_BITMAPS 14


#define EVENT_TABULEIRO _T("EVENT_TABULEIRO")					//evento tabuleiro
#define EVENT_NAMEDPIPE_SV _T("EVENT_NAMEDPIPE_SV")				//nome evento named pipe servidor
#define EVENT_NAMEDPIPE_CLI _T("EVENT_NAMEDPIPE_CLI")			//nome evento named pipe cliente

#define MUTEX_NPIPE_SV _T("MUTEX_NPIPE_SV")						//nome mutex named Pipe servidor
#define MUTEX_NPIPE_CLI _T("MUTEX_NPIPE_CLI")					//nome mutex named Pipe cliente

#define INFO _T("INFO")
#define PECA _T("PECA")
#define SEQ _T("SEQ")
#define SUSPENDE _T("SUSPENDE")
#define RETOMA _T("RETOMA")
#define SAIR _T("SAIR")
#define JOGOSINGLEP _T("JOGOSINGLEP")
#define JOGOMULTIP _T("JOGOMULTIP")
#define JOGOMULTIPCANCEL _T("JOGOMULTIPCANCEL")
#define INICIAJOGO _T("INICIAJOGO")
#define GANHOU _T("GANHOU")
#define PERDEU _T("PERDEU")
#define PROXNIVEL _T("PROXNIVEL")

#define MUTEX_BITMAP _T("MUTEX_BITMAP")								//nome mutex named Pipe servidor

#define NOME_PIPE_CLIENTE _T("\\\\.\\pipe\\cliente")				//nome named pipe Cliente-Servidor
#define NOME_PIPE_SERVIDOR _T("\\\\.\\pipe\\servidor")				//nome named pipe Servidor-Cliente


typedef struct {
	TCHAR cmd[MAX];									//comando da mensagem enviada. Ex: paraAgua, info, cliquedir
	TCHAR args[MAX][TAM_BUFFER];					//argumentos associados aos comandos, pode ter ou não
	int numargs;
}Msg;

typedef struct {
	TCHAR nome[256];
}DATA;

typedef struct {
	HANDLE hPipe;									// handle do pipe
	BOOL activo;									//representa se a instancia do named pipe está ou nao ativa, se ja tem um cliente ou nao
}PipeDados;

typedef struct {
	PipeDados hPipe;
	HANDLE hEventoNamedPipe;
	HANDLE hMutex;
	HANDLE hMutexBitmap;
	HANDLE hThreadLer;
	HANDLE hThreadEscrever;
	int terminar;
	TCHAR mensagem[MAX];
	int tabuleiro[20][20];
	int tamX, tamY;
	int celulaAtivaX, celulaAtivaY;
	HWND hWnd;
	int seq[6];
	BOOL jogoCorrer;
	BOOL texturas;
	TCHAR info[MAX];
	HANDLE hMessageBox;
	int vitorias;
}DadosThreadPipe;


#endif