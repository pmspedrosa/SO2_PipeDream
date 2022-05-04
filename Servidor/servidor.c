#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <time.h>
#define MAX 256
#define N_STR 5
#define TAM_BUFFER 10

#define MEM_PARTILHADA _T("MEM_PARTILHADA")			//nome da mem partilhada
#define MUTEX _T("MUTEX")							//nome da mutex
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura



/*comandos*/
#define PFAGUA _T("pfagua")							//nome do semaforo de leitura
#define BARR _T("barr")								//nome do semaforo de leitura



//━ = 1,┃ = 2, ┏ = 3, ┓ = 4, ┛ = 5, ┗ = 6

typedef struct {									//estrutura que vai criar cada celula do buffer circular
	int id;											//id do produtor 
	char comando[MAX];								//comando produzido
	char args[N_STR][MAX];							//argumentos caso existam (args[NUM_STRINGS][MAX_TAM])
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
	char cmd[MAX];

	while (dados->terminar == 0) {	//termina quando dados.terminar != 0

		WaitForSingleObject(dados->hSemLeitura, INFINITE);	//bloqueia à espera que o semaforo fique assinalado
		WaitForSingleObject(dados->hMutex, INFINITE);		//bloqueia à espera do unlock do mutex
		
		_tcscpy_s(cmd, 100,celula.comando);				//(destination, sizeinwords,source)
		
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

		_tprintf(_T("Servidor consumiu comando [%s] de monitor [%d]\n", celula.comando, celula.id));

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

	dados->hSemLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER, SEM_LEITURA);

	if (dados->hSemLeitura == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n", GetLastError()));
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

	hThread = CreateThread(NULL, 0, ThreadConsumidor, &dados, 0, NULL);

	if (hThread != NULL) {
		_tprintf(_T("Escreva 'exit' para sair.\n"));
		do { _getts_s(comando, 100); } while (_tcscmp(comando, _T("exit")) != 0);
		dados.terminar = 1;
		WaitForSingleObject(hThread, INFINITE);
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