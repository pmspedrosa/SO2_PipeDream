#include "monitor.h"


DWORD WINAPI ThreadProdutor(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	CelulaBuffer celula;
	TCHAR comando[MAX];
	//TCHAR* prox_tok;	//A char* stores the starting memory location of a C-string
	//TCHAR* delim = _T(" ");	//delimitador para string tokenizer
	HANDLE hSemEscritaWait[2] = { dados->hSemEscrita, dados->hEventTerminar };

	unsigned int cont = 0;

	while (dados->terminar == 0) {
		celula.id = dados->id;

		if (WaitForMultipleObjects(2, hSemEscritaWait, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {		//bloqueia à espera que o semaforo fique assinalado
			return 0;
		}

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

		_tcscpy_s(celula.comando, MAX, comando);


		WaitForSingleObject(dados->hMutexBufferCircular, INFINITE);

		CopyMemory(&dados->memPar->buffer[dados->memPar->posE], &celula, sizeof(CelulaBuffer));
		dados->memPar->posE++;
		if (dados->memPar->posE == TAM_BUFFER) //chegou ao limite? Sim, volta a 0
			dados->memPar->posE = 0;

		ReleaseMutex(dados->hMutexBufferCircular);
		ReleaseSemaphore(dados->hSemLeitura, 1, NULL); //o que vai fazer a leitura vai ter oportunidade de ler 1 bloco

		_tprintf(TEXT("P%d produziu %s.\n"), dados->id, celula.comando);
		//Sleep(4 * 1000); //pedido pelo ex
	}
	ReleaseMutex(dados->hMutexBufferCircular);
	ReleaseSemaphore(dados->hSemLeitura, 1, NULL);
	return 0;
}

VOID displayTabuleiro(int tab[20][20], int tamX, int tamY, HANDLE hConsole, WORD atributosBase) {
	for (int i = 0; i < tamX; i++) {
		_tprintf(_T("____"));
	}
	_tprintf(_T("\n"));

	for (int y = 0; y < tamY; y++) {
		for (int x = 0; x < tamX; x++) {
			_tprintf(_T("|   "));
		}
		_tprintf(_T("|\n"));

		for (int x = 0; x < tamX; x++) {
			switch (tab[x][y]) {
			case 0:
				_tprintf(_T("|   "));
				break;
			case 1:
				_tprintf(_T("| ━ "));
				break;
			case 2:
				_tprintf(_T("| ┃ "));
				break;
			case 3:
				_tprintf(_T("| ┏ "));
				break;
			case 4:
				_tprintf(_T("| ┓ "));
				break;
			case 5:
				_tprintf(_T("| ┛ "));
				break;
			case 6:
				_tprintf(_T("| ┗ "));
				break;
			case 7:
				_tprintf(_T("| █ "));
				break;
			case -1:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("━ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			case -2:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("┃ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			case -3:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("┏ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			case -4:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("┓ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			case -5:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("┛ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			case -6:
				_tprintf(_T("| "));
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
				_tprintf(_T("┗ "));
				SetConsoleTextAttribute(hConsole, atributosBase);
				break;
			};
		}
		_tprintf(_T("|\n"));

		for (int x = 0; x < tamX; x++) {
			_tprintf(_T("|___"));
		}
		_tprintf(_T("|\n"));
	}
}

VOID updateDisplay(DadosThread* dados, HANDLE hConsole, WORD atributosBase) {
	//system("cls");
	if (dados->memPar->dadosTabuleiro1.jogadorAtivo){
		_tprintf(_T("TABULEIRO 1\n"));
		displayTabuleiro(dados->memPar->dadosTabuleiro1.tabuleiro, dados->memPar->tamX, dados->memPar->tamY, hConsole, atributosBase);
	}
	else {
		_tprintf(_T("TABULEIRO 1 não ativo\n"));
	}
	if (dados->memPar->dadosTabuleiro2.jogadorAtivo) {
		_tprintf(_T("\n\n\nTABULEIRO 2\n"));
		displayTabuleiro(dados->memPar->dadosTabuleiro2.tabuleiro, dados->memPar->tamX, dados->memPar->tamY, hConsole, atributosBase);
	}
	_tprintf(_T("\n"));
	TCHAR estado[MAX];
	_tcscpy_s(estado, MAX, dados->memPar->estado);
	_tprintf(_T("%s\n"), estado);
}

DWORD WINAPI ThreadDisplay(LPVOID params) {
	DadosThread* dados = (DadosThread*)params;
	HANDLE hEventUpdateTabuleiro = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TABULEIRO);
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD atributosBase;
	HANDLE hEventsWait[2] = { hEventUpdateTabuleiro, dados->hEventTerminar };
	DWORD waitRet;

	/* Save current attributes */
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	atributosBase = consoleInfo.wAttributes;

	if (hEventUpdateTabuleiro == NULL) {
		_tprintf(_T("Erro: OpenEvent (%d)\n"), GetLastError());
		dados->terminar = 1;
		return 0;
	}
	HANDLE hMutexTabuleiro = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_TABULEIRO);
	if (hEventUpdateTabuleiro == NULL) {
		_tprintf(_T("Erro: OpenMutex (%d)\n"), GetLastError());
		return -1;
	}

	WaitForSingleObject(hMutexTabuleiro, INFINITE);
	updateDisplay(dados, hConsole, atributosBase);
	ReleaseMutex(hMutexTabuleiro);
	while (!dados->terminar)
	{
		waitRet = WaitForMultipleObjects(2, hEventsWait, FALSE, INFINITE);

		if (waitRet == WAIT_OBJECT_0 + 1){
			dados->terminar = 1;
			break;
		}
		else if(waitRet != WAIT_OBJECT_0){
			_tprintf(_T("Erro: WaitForMultipleObject (%d)\n"), GetLastError());
			return -1;
		}
		WaitForSingleObject(hMutexTabuleiro, INFINITE);
		updateDisplay(dados, hConsole, atributosBase);
		ReleaseMutex(hMutexTabuleiro);
		if (ResetEvent(hEventUpdateTabuleiro) == 0) {
			_tprintf(_T("Erro: ResetEvent (%d)\n"), GetLastError());
			return -1;
		}
	}
	CloseHandle(hEventUpdateTabuleiro);
	CloseHandle(hMutexTabuleiro);
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
		_tprintf(_T("Erro: MapViewOfFile (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		return FALSE;
	}

	if (primeiroProcesso) {
		dados->memPar->nP = 0;
		dados->memPar->posE = 0;
		dados->memPar->posL = 0;
	}
	dados->hMutexBufferCircular = CreateMutex(NULL, FALSE, MUTEX_BUFFER);
	if (dados->hMutexBufferCircular == NULL) {
		_tprintf(_T("Erro: CreateMutex (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
		return FALSE;
	}

	dados->hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_ESCRITA);

	if (dados->hSemEscrita == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
		return FALSE;
	}

	dados->hSemLeitura = CreateSemaphore(NULL, 0, 1, SEM_LEITURA);

	if (dados->hSemLeitura == NULL) {
		_tprintf(_T("Erro: CreateSemaphore (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
		return FALSE;
	}

	dados->hEventTerminar = CreateEvent(NULL, TRUE, FALSE, EVENT_TERMINAR);
	if (dados->hEventTerminar == NULL) {
		_tprintf(_T("Erro: CreateEvent (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
		return FALSE;
	}


	return TRUE;
}




int _tmain(int argc, TCHAR* argv[]) {

	HANDLE hFileMap, hThread, hThreadDisplay;
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

	WaitForSingleObject(dados.hMutexBufferCircular, INFINITE); //incrementar o num de produtores

	dados.memPar->nP++;
	dados.id = dados.memPar->nP;
	ReleaseMutex(dados.hMutexBufferCircular);

	hThread = CreateThread(NULL, 0, ThreadProdutor, &dados, 0, NULL);
	hThreadDisplay = CreateThread(NULL, 0, ThreadDisplay, &dados, 0, NULL);

	while (dados.terminar == 0) {

	}
	
	HANDLE hThreadsWait[] = { hThread, hThreadDisplay};
	WaitForMultipleObjects(2, hThreadsWait, TRUE, 100);


	UnmapViewOfFile(dados.memPar);
	//CloseHandles...
	CloseHandle(dados.hMutexBufferCircular);
	CloseHandle(dados.hSemEscrita);
	CloseHandle(dados.hSemLeitura);
	CloseHandle(dados.hMapFile);
	CloseHandle(dados.hEventTerminar);
	CloseHandle(hThread);

	return 0;
}