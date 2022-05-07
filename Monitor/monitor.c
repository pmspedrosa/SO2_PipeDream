#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
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
#define PFAGUA _T("PFAGUA")							//nome do semaforo de leitura
#define BARR _T("BARR")								//nome do semaforo de leitura
#define SAIR _T("SAIR")



typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	TCHAR* comando;									//comando produzido
	TCHAR* args[N_STR];								//argumentos caso existam (args[NUM_STRINGS][MAX_TAM])
}CelulaBuffer;

typedef struct {
	int nP;											//numero de produtores
	int posE;										//posicao de escrita
	int	posL;										//posicao de leitura
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


DWORD WINAPI ThreadProdutor(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	CelulaBuffer celula;
	TCHAR comando[MAX];
	TCHAR* delim = " ";	//delimitador para string tokenizer
	TCHAR *prox_tok;	//A char* stores the starting memory location of a C-string
	unsigned int cont = 0;

	while (dados->terminar == 0) {
		celula.id = dados->id;

		WaitForSingleObject(dados->hSemEscrita, INFINITE);
		WaitForSingleObject(dados->hMutex, INFINITE);

		//inicializa dados celula
		celula.comando = NULL;
		for (int i = 0; i < N_STR; i++)
		{
			celula.args[i] = NULL;
		}
		prox_tok = NULL;
		
		//fica a espera dos comandos

		fflush(stdin);
		_fgetts(comando, MAX, stdin);
		//Retirar \n
		comando[_tcslen(comando) - 1] = '\0';
		//Maiúsculas
		for (int i = 0; i < _tcslen(comando); i++)
			comando[i] = _totupper(comando[i]);
		_tprintf(TEXT("Comando : %s\n"), comando);
		if (_tcscmp(comando, SAIR) == 0) {
			dados->terminar = 1;
			continue;
		}
		if (_tcslen(comando) != 0 || comando == NULL) {
			//comando geral
			TCHAR* ptr = wcstok_s(comando, delim, &prox_tok);
			if (ptr != NULL)
				celula.comando = ptr;
			//args
			while (ptr != NULL || cont <= N_STR)	//se cont == N_STR, argumentos a mais são ignorados
			{
				ptr = wcstok_s(NULL, delim, &prox_tok);
				if (ptr != NULL)
					celula.args[cont] = ptr;
				cont++;
			}
			//_tprintf(TEXT("TOKENS %s .\n"),ptr);

		}



		ReleaseMutex(dados->hMutex);
		ReleaseSemaphore(dados->hSemLeitura, 1, NULL); //o que vai fazer a leitura vai ter oportunidade de ler 1 bloco

		_tprintf(TEXT("P%d produziu .\n"), dados->id);
		//Sleep(4 * 1000); //pedido pelo ex
	}
	ReleaseMutex(dados->hMutex);
	ReleaseSemaphore(dados->hSemLeitura, 1, NULL);
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
	dados->memPar = (MemPartilhada*)MapViewOfFile(dados->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (dados->memPar == NULL) {
		_tprintf(_T("Erro: MapViewOfFile (%d)\n", GetLastError()));
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




int _tmain(int argc, TCHAR* argv[]) {

	HANDLE hFileMap, hThread;
	DadosThread dados;
	BOOL primeiroProcesso = FALSE;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	

	dados.terminar = 0;
	if (!initMemAndSync(&dados)) {
		_tprintf(_T("Erro ao criar/abrir a memoria partilhada e mecanismos sincronos.\n"));
		exit(1);
	}

	WaitForSingleObject(dados.hMutex, INFINITE); //incrementar o num de produtores
	dados.memPar->nP++;
	dados.id = dados.memPar->nP;
	ReleaseMutex(dados.hMutex);

	hThread = CreateThread(NULL, 0, ThreadProdutor, &dados, 0, NULL);

	while (dados.terminar == 0) {
		
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