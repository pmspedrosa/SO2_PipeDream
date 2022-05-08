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
#define PFAGUA _T("PFAGUA")							//nome do semaforo de leitura
#define BARR _T("BARR")								//nome do semaforo de leitura
#define SAIR _T("SAIR")


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

/*THREAD CONSUMIDOR*/
/*
Consome os comandos lançados pelo Produtor
Monitor = M, Cliente = C
Comandos possiveis:
	- Parar água por periode de tempo	 [pfagua arg1 ...]	(M)
	- Adicionar barreira intransponivel	 [barr arg1 ...]	(M)
	- 


semaforo
mutex
processamento de comandos -> lançamento de funções especificas
releasemutex
releasesemaforo
mensagem debug com o que consumiu
*/

DWORD WINAPI ThreadConsumidor(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	CelulaBuffer celula;
	TCHAR cmd[MAX];

	while (dados->terminar == 0) {	//termina quando dados.terminar != 0

		WaitForSingleObject(dados->hSemLeitura, INFINITE);	//bloqueia à espera que o semaforo fique assinalado
		WaitForSingleObject(dados->hMutex, INFINITE);		//bloqueia à espera do unlock do mutex
		

		CopyMemory(&celula, &dados->memPar->buffer[dados->memPar->posL], sizeof(CelulaBuffer)); 
		dados->memPar->posL++; 
		if (dados->memPar->posL == TAM_BUFFER) 
			dados->memPar->posL = 0; 


		_tprintf(_T("P%d consumiu %s.\n"), celula.id, celula.comando);
		_tcscpy_s(cmd,MAX ,celula.comando);				//(destination, sizeinwords,source)
		

		
		if (_tcscmp(cmd, PFAGUA) == 0)						//comando para fluxo agua por determinado tempo
		{
			// func pfagua com args ...
		}
		else if (_tcscmp(cmd, BARR) == 0)					//comando adicionar barreira
		{
			// func barr com args ...
		}
		else //comando nao encontrado
		{
			// comando desconhecido
		}

		ReleaseMutex(dados->hMutex);						//liberta mutex
		ReleaseSemaphore(dados->hSemEscrita, 1, NULL);		//liberta semaforo

		//_tprintf(_T("Servidor consumiu comando [%s] de monitor [%d]\n", celula.comando, celula.id));

	}//while
	return 0;
}




BOOL initMemAndSync(DadosThread* dados, unsigned int tamH, unsigned int tamV) {
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
		_tprintf(_T("Erro: MapViewOfFile (%d)\n", GetLastError()));
		UnmapViewOfFile(dados->hMapFile);
		return FALSE;
	}

	dados->memPar->tamX = tamH;
	dados->memPar->tamY = tamV;

	if (primeiroProcesso) {
		dados->memPar->nP = 0;
		dados->memPar->posE = 0;
		dados->memPar->posL = 0;
	}
	dados->hMutex = CreateMutex(NULL, FALSE, MUTEX);
	if (dados->hMutex == NULL) {
		_tprintf(_T("Erro: CreateMutex (%d)\n", GetLastError()));
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}

	dados->hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_ESCRITA);

	if (dados->hSemEscrita == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n", GetLastError()));
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}

	dados->hSemLeitura = CreateSemaphore(NULL, 0, 1, SEM_LEITURA);

	if (dados->hSemLeitura == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n", GetLastError()));
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
	}
	return TRUE;
}


DWORD carregaValorConfig(TCHAR valorString[], HKEY hChaveRegistry, TCHAR nomeValorRegistry[], unsigned int valorOmissao, unsigned int* varGuardar, unsigned int min, unsigned int max) {
	unsigned int valorNum = 0;
	unsigned int sizeValor;
	if(valorString != NULL){	
		valorNum = _ttoi(valorString);
	}
	if (valorNum < min || valorNum > max) {
		sizeValor = sizeof(valorNum);
		if (RegQueryValueEx(hChaveRegistry, nomeValorRegistry, NULL, NULL, &valorNum, &sizeValor) == ERROR_SUCCESS) {
			*varGuardar = valorNum;
			return 1;
		}
		else {
			*varGuardar = valorOmissao;
			RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(unsigned int));
			return 2;
		}
	}
	else {
		*varGuardar = valorNum;
		RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(unsigned int));
		return 3;
	}
}


int _tmain(int argc, LPTSTR argv[]) {
	HKEY hChaveRegistry;
	DWORD respostaRegistry;
	DWORD longGenerico1, longGenerico2;

	//n�o sei como planeamos guardar esta informa��o. Provavelmente uma estrutura de dados
	DWORD tamHorizontal, tamVertical, tempoInicioAgua;

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

  if (RegCreateKeyEx(HKEY_CURRENT_USER, CHAVE_REGISTRY_NOME, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hChaveRegistry, &respostaRegistry) != ERROR_SUCCESS) {
		DWORD error = GetLastError();
		_tprintf(TEXT("Erro a abrir a chave [%d]\n"), hChaveRegistry);
		return -1;
	}
	if (argc >= 4) {
		carregaValorConfig(argv[1], hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal, 3, 20);
		carregaValorConfig(argv[2], hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical, 3, 20);
		carregaValorConfig(argv[3], hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &tempoInicioAgua, 5, 45);
	}
	else {
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal, 3, 20);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical, 3, 20);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &tempoInicioAgua, 5, 45);
	}

	_tprintf(TEXT("tam_h -> %d  \ntam_v -> %ld\ntempo -> %ld\n"), tamHorizontal, tamVertical, tempoInicioAgua);


	RegCloseKey(hChaveRegistry);
  
	dados.terminar = 0;
	//dados.memPar->tabuleiro = *tab;
	if (!initMemAndSync(&dados, tamHorizontal, tamVertical)) {
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