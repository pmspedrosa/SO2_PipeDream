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
#define MUTEX _T("MUTEX")							//nome da mutex
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")							//nome do semaforo de servidor



/*comandos*/
#define PFAGUA _T("PFAGUA")							//nome comando para fluxo agua durante um periodo de tempo
#define BARR _T("BARR")								//nome comando adiciona barreira á grelha de jogo		
#define MODO _T("MODO")								//nome comando modo sequencia de peças/tubos
#define SAIR _T("SAIR")								//sair


typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	TCHAR comando[MAX];								//comando produzido
}CelulaBuffer;

typedef struct {
	unsigned int nP;								//numero de produtores
	unsigned int posE;								//posicao de escrita
	unsigned int posL;								//posicao de leitura
	int tabuleiro[20][20];							//tabuleiro jogo
	unsigned int tamX, tamY;						//tam tabuleiro
	unsigned int posX, posY;						//posição da água
	CelulaBuffer buffer[TAM_BUFFER];
}MemPartilhada;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hSemServidor;							//semaforo para controlo do número de svs ativos
	HANDLE hMutex;									//mutex
	HANDLE hMapFile;								//map file
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
}DadosThread;



TCHAR** divideString(TCHAR * comando, const TCHAR * delim, unsigned int* tam) {
	TCHAR* proxToken = NULL, ** temp, ** arrayCmd = NULL;
	TCHAR* token = _tcstok_s(comando, delim, &proxToken);

	if (comando == NULL || _tcslen(comando) == 0) {
		_ftprintf(stderr, TEXT("[ERRO] String comando vazia!"));
		return NULL;
	}

	*tam = 0;

	while (token != NULL) {
		//realocar a memória para integrar mais um argumento
		//realloc returna um ponteiro para a nova memoria alocada, ou NULL quando falha
		temp = realloc(arrayCmd, sizeof(TCHAR*) * (*tam + 1));		

		if (temp == NULL) {
			_ftprintf(stderr, TEXT("[ERRO] Falha ao alocar memoria!"));
			return NULL;
		}
		arrayCmd = temp;						//apontar para a nova memoria alocada				
		arrayCmd[(*tam)++] = token;

		token = _tcstok_s(NULL, delim, &proxToken);
	}

	return arrayCmd;
}





DWORD WINAPI ThreadConsumidor(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	CelulaBuffer celula;
	TCHAR comando[MAX], ** arrayComandos = NULL;
	const TCHAR delim[2] = _T(" ");
	unsigned int nrArgs = 0;

	while (dados->terminar == 0) {	//termina quando dados.terminar != 0

		WaitForSingleObject(dados->hSemLeitura, INFINITE);	//bloqueia à espera que o semaforo fique assinalado
		WaitForSingleObject(dados->hMutex, INFINITE);		//bloqueia à espera do unlock do mutex
		

		CopyMemory(&celula, &dados->memPar->buffer[dados->memPar->posL], sizeof(CelulaBuffer)); 
		dados->memPar->posL++; 
		if (dados->memPar->posL == TAM_BUFFER) 
			dados->memPar->posL = 0; 

		_tcscpy_s(comando, MAX, celula.comando);
		_tprintf(_T("P%d consumiu %s.\n"), celula.id, celula.comando);

		free(arrayComandos);												//libertar a memoria de args

		arrayComandos = divideString(comando, delim, &nrArgs);
		

		
		if (arrayComandos != NULL ) {
			_tprintf(_T("\ncomando: %s.\n"), arrayComandos[0]);
			for (int i = 0; i < nrArgs; i++)
			{
				_tprintf(_T("arg[%d]: %s.\n"), i, arrayComandos[i]);
			}
			if (_tcscmp(arrayComandos[0], PFAGUA) == 0)						//comando para fluxo agua por determinado tempo
			{
				if (nrArgs < 1)
					continue;
				// func pfagua com args ...
			}
			else if (_tcscmp(arrayComandos[0], BARR) == 0)					//comando adicionar barreira
			{
				_tprintf(_T("barrrrrr\n"));
				// func barr com args ...
				if (nrArgs < 2)
					continue;
			}
			else if (_tcscmp(arrayComandos[0], MODO) == 0)					//comando alterar modo sequencia tubos
			{
				_tprintf(_T("modooooooo\n"));
				// func mudar modo sequencia peças/tubos ...
			}
			else //comando nao encontrado
			{
				_tprintf(_T("Comando desconhecido!\n"));// func barr com args ...
				// comando desconhecido
			}
		}


		ReleaseMutex(dados->hMutex);						//liberta mutex
		ReleaseSemaphore(dados->hSemEscrita, 1, NULL);		//liberta semaforo
		//_tprintf(_T("Servidor consumiu comando [%s] de monitor [%d]\n", celula.comando, celula.id));

	}//while
	return 0;
}






BOOL initMemAndSync(DadosThread* dados) {
	BOOL primeiroProcesso = FALSE;
	dados->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MEM_PARTILHADA); //verificar se ja existe, senao existir, pomos as variaveis a 0
	if (dados->hMapFile == NULL)
	{
		primeiroProcesso = TRUE;
		_tprintf(_T("Vou criar um novo ficheiro partilhado.\n"));
		dados->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MemPartilhada), MEM_PARTILHADA);
		if (dados->hMapFile == NULL) {
			_tprintf(TEXT("Erro no CreateFileMapping\n"));
			return FALSE;
		}
	}
	dados->memPar = (MemPartilhada*)MapViewOfFile(dados->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MemPartilhada));

	if (dados->memPar == NULL) {
		_tprintf(_T("Erro: MapViewOfFile (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		return FALSE;
	}

	if (primeiroProcesso) {
		dados->memPar->nP = 0;
		dados->memPar->posE = 0;
		dados->memPar->posL = 0;
	}
	dados->hMutex = CreateMutex(NULL, FALSE, MUTEX);
	if (dados->hMutex == NULL) {
		_tprintf(_T("Erro: CreateMutex (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}

	dados->hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_ESCRITA);

	if (dados->hSemEscrita == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}

	dados->hSemLeitura = CreateSemaphore(NULL, 0, 1, SEM_LEITURA);

	if (dados->hSemLeitura == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}
	return TRUE;
}




int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hThread;
	DadosThread dados;
	TCHAR comando[100];
	//unsigned int tab[20][20];

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	/*Semaforo para garantir a existência de apenas 1 servidor ativo*/
	dados.hSemServidor = CreateSemaphore(NULL, NUM_SV, NUM_SV, SEM_SV);
	
	if (dados.hSemServidor == NULL) {
		_tprintf(TEXT("Erro no CreateSemaphore\n"));
		return -1;
	}
	if (WaitForSingleObject(dados.hSemServidor, MAX) == WAIT_TIMEOUT) {
		_tprintf(TEXT("Já existe um servidor ativo ... \n"));
		CloseHandle(dados.hSemServidor);
		return 0;
	}

	dados.terminar = 0;
	//dados.memPar->tabuleiro = *tab;
	if (!initMemAndSync(&dados)) {
		_tprintf(_T("Erro ao criar/abrir a memoria partilhada e mecanismos sincronos.\n"));
		exit(1);
	}

	hThread = CreateThread(NULL, 0, ThreadConsumidor, &dados, 0, NULL);

	if (hThread != NULL) {
		_tprintf(_T("Escreva 'SAIR' para sair.\n"));
		do { 
			fflush(stdin);
			_fgetts(comando, MAX-1, stdin);
			//Retirar \n
			comando[_tcslen(comando) - 1] = '\0';
			//Maiúsculas
			for (int i = 0; i < _tcslen(comando); i++)
				comando[i] = _totupper(comando[i]);
			_tprintf(TEXT("Comando : %s\n"), comando);
		} while (_tcscmp(comando, SAIR) != 0);
		dados.terminar = 1;
		WaitForSingleObject(hThread, 100);
	}

	UnmapViewOfFile(dados.memPar);
	//CloseHandles...
	CloseHandle(dados.hMutex);
	CloseHandle(dados.hSemEscrita);
	CloseHandle(dados.hSemLeitura);
	CloseHandle(dados.hMapFile);
	CloseHandle(hThread);
	

	return 0;
}





BOOL verifica(DadosThread* dados) {
	MemPartilhada* mem = dados->memPar;
	unsigned int tab = mem->tabuleiro;
	int* x = mem->posX;
	int* y = mem->posY;
	int maxX = dados->memPar->tamX, maxY = dados->memPar->tamY;

	/*****************************/
	/*verifica se peça horizontal*/
	/*****************************/
	if (tab[x][y] == 1) {	
		//verificar se bate contra a barreira do jogo[horizontal]
		if ((x == 0 && tab[x+1][y] == 8) || (x == maxX && tab[x - 1][y] == 8)) {	
			return FALSE;
		}
		//verifica peça direita [agua esquerda]
		else if (x==0 || (tab[x-1][y] == 8)) {		
			if (tab[x + 1][y] == 1 || tab[x + 1][y] == 4 || tab[x + 1][y] == 5)
				return TRUE;
		}
		//verifica peça esquerda [agua direita]
		else if (x == maxX || (tab[x + 1][y] == 8)) {
			if (tab[x - 1][y] == 1 || tab[x - 1][y] == 3 || tab[x - 1][y] == 6)
				return TRUE;
		}	

	/***************************/
	/*verifica se peça vertical*/
	/***************************/
	}else if (tab[x][y] == 2) {	
		//verificar se bate contra a barreira do jogo[vertical]
		if ((y == 0 && tab[x][y+1] == 8) || (y == maxY && tab[x][y-1] == 8)) {
			return FALSE;
		}
		//verificar peça baixo[agua cima]
		else if (y==0 || (tab[x][y-1] == 8)) {		
		if (tab[x][y+1] == 2 || tab[x][y+1] == 5 || tab[x][y+1] == 6)
			return TRUE;
		}
		//verificar peça cima[agua baixo]
		else if (y == maxY || (tab[x][y + 1] == 8)) {
			if (tab[x][y - 1] == 2 || tab[x][y - 1] == 3 || tab[x][y - 1] == 4)
				return TRUE;
		}

	/************************************/
	/*verifica se peça 90º direita baixo*/
	/************************************/
	}else if (tab[x][y] == 3) {	
		//verificar se bate contra a barreira do jogo[direita, baixo]
		if ((x == maxX && y == maxY) || (x == maxX && tab[x][y+1] == 8) || (y == maxY && tab[x+1][y] == 8)) {
			return FALSE;
		}
		//verificar peça direita[agua baixo]
		else if (y==maxY || (tab[x][y+1] == 8)) {		
			if (tab[x+1][y] == 1 || tab[x+1][y] == 4 || tab[x+1][y] == 5)
				return TRUE;
		}
		//verificar peça baixo[agua direita]
		else if (x == maxX || (tab[x+1][y] == 8)) {
			if (tab[x][y + 1] == 2 || tab[x][y + 1] == 5 || tab[x][y + 1] == 6)
				return TRUE;
		}

	/*************************************/
	/*verifica se peça 90º esquerda baixo*/
	/*************************************/
	}else if (tab[x][y] == 4) {	
		//verificar se bate contra a barreira do jogo[esquerda, baixo]
		if ((x == 0 && y == maxY) || (x == 0 && tab[x][y+1] == 8) || (y == maxY && tab[x-1][y] == 8)) {
			return FALSE;
		}
		//verificar peça esquerda[agua baixo]
		else if (y == maxY || (tab[x][y+1] == 8)) {
			if (tab[x - 1][y] == 1 || tab[x - 1][y] == 3 || tab[x - 1][y] == 6)
				return TRUE;
		}
		//verificar peça baixo[agua esquerda]
		else if (x == 0 || (tab[x - 1][y] == 8)) {
			if (tab[x][y + 1] == 2 || tab[x][y + 1] == 5 || tab[x][y + 1] == 6)
				return TRUE;
		}

	/*************************************/
	/*verifica se peça 90º esquerda cima*/
	/*************************************/
	}else if (tab[x][y] == 5) {
		//verificar se bate contra a barreira do jogo[esquerda, cima]
		if ((x == 0 && y == 0) || (x == 0 && tab[x][y - 1] == 8) || (y == 0 && tab[x - 1][y] == 8)) {
			return FALSE;
		}
		//verificar peça esquerda[agua cima]
		else if (x == 0 || (tab[x][y-1] == 8)) {
			if (tab[x - 1][y] == 1 || tab[x - 1][y] == 3 || tab[x - 1][y] == 6)
				return TRUE;
		}
		//verificar peça cima[agua esquerda]
		else if (y == 0 || (tab[x-1][y] == 8)) {
			if (tab[x][y - 1] == 2 || tab[x][y - 1] == 3 || tab[x][y - 1] == 4)
				return TRUE;
		}

	/************************************/
	/*verifica se peça 90º direita cima*/
	/************************************/
	}else if (tab[x][y] == 6) {	
		//verificar se bate contra a barreira do jogo[direita, cima]
		if ((x == maxX && y == 0) || (x == maxX && tab[x][y - 1] == 8) || (y == 0 && tab[x + 1][y] == 8)) {
			return FALSE;
		}
		//verificar peça direita[agua cima]
		else if (y==0 || (tab[x][y-1] == 8)) {		
			if (tab[x + 1][y] == 1 || tab[x + 1][y] == 4 || tab[x + 1][y] == 5)
				return TRUE;
		}
		//verificar peça cima[agua direita]
		else if (x = maxX || (tab[x + 1][y] == 8)) {
			if (tab[x][y - 1] == 2 || tab[x][y - 1] == 3 || tab[x][y - 1] == 4)
				return TRUE;
		}
	}
	return FALSE;
}