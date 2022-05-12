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
#define MUTEX _T("MUTEX")							//nome da mutex
#define SEM_ESCRITA _T("SEM_ESCRITA")				//nome do semaforo de escrita
#define SEM_LEITURA _T("SEM_LEITURA")				//nome do semaforo de leitura
#define SEM_SV _T("SEM_SV")							//nome do semaforo de servidor

#define CIMA 0
#define DIREITA 1
#define BAIXO 2
#define ESQUERDA 3


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
}MemPartilhada;

typedef struct {
	int (*tabuleiro)[20][20];						//ponteiro para o tabuleiro referente na memória partilhada // TER CUIDADO QUANDO SE DESMAPEAR A MEMÓRIA VIRTUAL
	int posX, posY;									//posição da água
	unsigned int dirAgua;							//direção da água	// 0 > cima , 1 > direita, 2 > baixo, 3 > esquerda
}DadosTabuleiro;

typedef struct {									//estrutura para passar as threads
	MemPartilhada* memPar;
	HANDLE hSemEscrita;								//semaforo que controla as escritas
	HANDLE hSemLeitura;								//semaforo que controla as leituras
	HANDLE hSemServidor;							//semaforo para controlo do número de svs ativos
	HANDLE hMutex;									//mutex
	HANDLE hMapFile;								//map file
	int terminar;									//flag para indicar a thread para terminar -> 1 para sair, 0 caso contrario
	int id;											//id do produtor
	DWORD tempoInicioAgua;							//tempo até água começar a fluir
	DadosTabuleiro tabuleiro1, tabuleiro2;
	//unsigned int dirAgua;							//direção da água	// 0 > cima , 1 > direita, 2 > baixo, 3 > esquerda
	//int posX, posY;									//posição da água
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

void carregaMapaPreDefinido(DadosThread* dados, int tabuleiro[20][20]) {
	tabuleiro[0][2] = 1;
	tabuleiro[1][2] = 1;
	tabuleiro[2][2] = 1;
	tabuleiro[3][2] = 1;
	tabuleiro[4][2] = 4;
	tabuleiro[4][3] = 2;
	tabuleiro[4][4] = 5;
	tabuleiro[3][4] = 3;
	tabuleiro[3][5] = 2;
	tabuleiro[3][6] = 6;
	tabuleiro[4][6] = 1;
	tabuleiro[5][6] = 1;
	tabuleiro[6][6] = 1;
	tabuleiro[7][6] = 1;
	tabuleiro[8][6] = 5;
	tabuleiro[8][5] = 3;
	tabuleiro[9][5] = 1;

	dados->tabuleiro1.posX = 0;
	dados->tabuleiro1.posY = 2;
	dados->tabuleiro1.dirAgua = DIREITA;
}

BOOL definirInicioFim(DadosThread* dados) {
	srand(time(0));
	
	int parede = (rand() % (4));				//define em que parede vai estar :  0 - cima // 1 - direita //2 - baixo // 3 - esquerda
	int min = 0, max, tipoTubo, posInicio, posFim;

	switch (parede){
	case 0:										//se estiver em cima ou em baixo, o limite máximo da sua posição é tamH, e o tubo inicial é vertical
	case 2:
		max = dados->memPar->tamX;
		tipoTubo = 2;
		break;
	case 1:										//se estiver à direita ou esquerda, o limite máximo da sua posição é tamV, e o tubo inicial é horizontal
	case 3:
		max = dados->memPar->tamY;
		tipoTubo = 1;
		break;
	default:									//se o valor aleatorio não for um dos esperados, retorna FALSE (apenas se ocorrer algum erro)
		return FALSE;
	}

	posInicio = (rand() % max);						//posição a ocupar, na parede 
	
	if (max%2 == 0){								//se max par
		if (posInicio >= max / 2) {						//e posInicio maior que metade, posFim tem de estar na primeira metade da parede
			min = max / 2;
		}
		else {											//e posInicio menor que metade, posFim tem de estar na segunda metade da parede
			max = max / 2;
		}
	}
	else {											//se max impar
		if (posInicio > max / 2) {						//e posInicio maior que metade inteira, posFim tem de estar na primeira metade da parede
			min = (max / 2) + 1;
		}
		else if (posInicio < max / 2) {					//e posInicio menor que metade inteira, posFim tem de estar na primeira metade da parede
			max = max / 2;						
		}												//se max impar e posInicio igual à metade inteira, posInicio está na posição do meio, logo posFim pode estar em qualquer posição da parede
	}

	posFim = ((rand() % (max - min))+min);
	int ultimaPos;

	switch (parede) {							
	case 0:																				//Parede de cima
		//dados->tabuleiro1.tabuleiro[posInicio][0] = tipoTubo;									//mete tubo correto na posição de inicio
		(*dados->tabuleiro2.tabuleiro)[posInicio][0] = tipoTubo;									//mete tubo correto na posição de inicio

		//dados->tabuleiro1.posX = posInicio;													//posição de agua ativa = posição de inicio
		//dados->tabuleiro1.posY = 0;
		dados->tabuleiro2.posX = posInicio;													//posição de agua ativa = posição de inicio
		dados->tabuleiro2.posY = 0;
		
		//dados->tabuleiro1.tabuleiro[posFim][dados->memPar->tamY - 1] = tipoTubo;		//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		(*dados->tabuleiro2.tabuleiro)[posFim][dados->memPar->tamY - 1] = tipoTubo;		//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		
		//dados->tabuleiro1.dirAgua = BAIXO;
		dados->tabuleiro2.dirAgua = BAIXO;

		break;
	case 1:
		ultimaPos = dados->memPar->tamX - 1;
		//dados->tabuleiro1.tabuleiro[ultimaPos][posInicio] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[ultimaPos][posInicio] = tipoTubo;

		//dados->tabuleiro1.posX = ultimaPos;
		//dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = ultimaPos;
		dados->tabuleiro2.posY = posInicio;
		
		//dados->tabuleiro1.tabuleiro[0][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[0][posFim] = tipoTubo;

		//dados->tabuleiro1.dirAgua = ESQUERDA;
		dados->tabuleiro2.dirAgua = ESQUERDA;

		break;
	case 2:
		ultimaPos = dados->memPar->tamY - 1;
		//dados->tabuleiro1.tabuleiro[posInicio][ultimaPos] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[posInicio][ultimaPos] = tipoTubo;

		//dados->tabuleiro1.posX = posInicio;
		//dados->tabuleiro1.posY = ultimaPos;
		dados->tabuleiro2.posX = posInicio;
		dados->tabuleiro2.posY = ultimaPos;

		//dados->tabuleiro1.tabuleiro[posFim][0] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[posFim][0] = tipoTubo;

		//dados->tabuleiro1.dirAgua = CIMA;
		dados->tabuleiro2.dirAgua = CIMA;
		
		break;
	case 3:
		//dados->tabuleiro1.tabuleiro[0][posInicio] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[0][posInicio] = tipoTubo;

		//dados->tabuleiro1.posX = 0;
		//dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = 0;
		dados->tabuleiro2.posY = posInicio;

		//dados->tabuleiro1.tabuleiro[dados->memPar->tamX - 1][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[dados->memPar->tamX - 1][posFim] = tipoTubo;

		//dados->tabuleiro1.dirAgua = DIREITA;
		dados->tabuleiro2.dirAgua = DIREITA;
		
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL fluirAgua(DadosThread* dados) {
	switch(dados->tabuleiro1.dirAgua){
	case CIMA:
		if (dados->tabuleiro1.posY - 1 < 0) {
			return FALSE;
		}
		switch ((*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY - 1]) {
		case 2:				// peça ┃
			_tprintf(_T("encontrei peça |, direcao: CIMA\n"));
			break;
		case 3:				// peça ┏
			dados->tabuleiro1.dirAgua = DIREITA;
			_tprintf(_T("encontrei peça r, direcao: DIREITA\n"));
			break;
		case 4:				// peça ┓
			dados->tabuleiro1.dirAgua = ESQUERDA;
			_tprintf(_T("encontrei peça q, direcao: ESQUERDA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dados->tabuleiro1.posY--;															//avança a posição ativa da água na direção de fluxo
		(*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case DIREITA:
		if (dados->tabuleiro1.posX + 1 >= dados->memPar->tamX) {
			return FALSE;
		}
		switch ((*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX+1][dados->tabuleiro1.posY]) {
		case 1:				// peça ━
			_tprintf(_T("encontrei peça -, direcao: DIREITA\n"));
			break;
		case 4:				// peça ┓
			dados->tabuleiro1.dirAgua = BAIXO;
			_tprintf(_T("encontrei peça q, direcao: BAIXO\n"));
			break;
		case 5:				// peça ┛
			dados->tabuleiro1.dirAgua = CIMA;
			_tprintf(_T("encontrei peça d, direcao: CIMA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dados->tabuleiro1.posX++;															//avança a posição ativa da água na direção de fluxo
		(*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case BAIXO:
		if (dados->tabuleiro1.posY + 1 >= dados->memPar->tamY) {
			return FALSE;
		}
		switch ((*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY + 1]) {
		case 2:				// peça ┃
			_tprintf(_T("encontrei peça |, direcao: BAIXO\n"));
			break;
		case 5:				// peça ┛
			dados->tabuleiro1.dirAgua = ESQUERDA;
			_tprintf(_T("encontrei peça d, direcao: ESQUERDA\n"));
			break;
		case 6:				// peça ┗
			dados->tabuleiro1.dirAgua = DIREITA;
			_tprintf(_T("encontrei peça L, direcao: DIREITA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dados->tabuleiro1.posY++;															//avança a posição ativa da água na direção de fluxo
		(*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case ESQUERDA:
		if (dados->tabuleiro1.posX - 1 < 0) {
			return FALSE;
		}
		switch ((*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX - 1][dados->tabuleiro1.posY]) {
		case 1:				// peça ━
			_tprintf(_T("encontrei peça -, direcao: ESQUERDA\n"));
			break;
		case 3:				// peça ┏
			dados->tabuleiro1.dirAgua = BAIXO;
			_tprintf(_T("encontrei peça r, direcao: BAIXO\n"));
			break;
		case 6:				// peça ┗
			dados->tabuleiro1.dirAgua = CIMA;
			_tprintf(_T("encontrei peça L, direcao: CIMA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dados->tabuleiro1.posX--;															//avança a posição ativa da água na direção de fluxo
		(*dados->tabuleiro1.tabuleiro)[dados->tabuleiro1.posX][dados->tabuleiro1.posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI ThreadAgua(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;

	_tprintf(_T("(ThreadAgua) Sleep %d"), dados->tempoInicioAgua * 1000);
	Sleep(dados->tempoInicioAgua * 1000);
	while (!dados->terminar) {
		if (fluirAgua(dados)) {
			_tprintf(_T("(ThreadAgua) Fluir água! Sleep %d"), TIMER_FLUIR * 1000);
			Sleep(TIMER_FLUIR * 1000);
		}else {
			_tprintf(_T("(ThreadAgua) FluirAgua -> FALSE"));
			break;
		}
	}
	_tprintf(_T("SAIR DA THREAD"));
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

	//if (primeiroProcesso) {		//desnecessário porque o servidor é sempre o primeiro programa lançado
		dados->memPar->nP = 0;
		dados->memPar->posE = 0;
		dados->memPar->posL = 0;
	//}
		
		//	Comentado por causa do mapa pré-definido da meta 1
		//dados->memPar->tamX = tamH;
		//dados->memPar->tamY = tamV;
		
		dados->memPar->tamX = 10;
		dados->memPar->tamY = 7;

		dados->tabuleiro1.tabuleiro = &(dados->memPar->tabuleiro1);
		dados->tabuleiro2.tabuleiro = &(dados->memPar->tabuleiro2);
		for (int i = 0; i < tamH; i++) {
			for (int j = 0; j < tamV; j++) {
				(*dados->tabuleiro1.tabuleiro)[i][j] = 0;

				(*dados->tabuleiro2.tabuleiro)[i][j] = 0;		//apenas meta 1. Depois será feito -> tabuleiro2 = tabuleiro1 (cópia de valores)
			}
		}

		//	MAPA PRÉ-DEFINIDO
		carregaMapaPreDefinido(dados, dados->tabuleiro1.tabuleiro);

		// Define início e fim
		//definirInicioFim(dados);	//meta 2 -> alterar para tabuleiro1


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

	DWORD tamHorizontal, tamVertical;

	HANDLE hThreadConsumidor, hThreadAgua;
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
		carregaValorConfig(argv[3], hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &(dados.tempoInicioAgua), 5, 45);
	}
	else {
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal, 3, 20);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical, 3, 20);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &(dados.tempoInicioAgua), 5, 45);
	}

	_tprintf(TEXT("tam_h -> %d  \ntam_v -> %ld\ntempo -> %ld\n"), tamHorizontal, tamVertical, dados.tempoInicioAgua);


	RegCloseKey(hChaveRegistry);
  
	dados.terminar = 0;
	//dados.memPar->tabuleiro = *tab;
	if (!initMemAndSync(&dados, tamHorizontal, tamVertical)) {
		_tprintf(_T("Erro ao criar/abrir a memoria partilhada e mecanismos sincronos.\n"));
		exit(1);
	}

	hThreadConsumidor = CreateThread(NULL, 0, ThreadConsumidor, &dados, 0, NULL);
	hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dados, 0, NULL);


	if (hThreadConsumidor != NULL && hThreadAgua != NULL) {
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
		WaitForSingleObject(hThreadConsumidor, 100);
	}

	UnmapViewOfFile(dados.memPar);
	//CloseHandles...
	CloseHandle(dados.hMutex);
	CloseHandle(dados.hSemEscrita);
	CloseHandle(dados.hSemLeitura);
	CloseHandle(dados.hMapFile);
	CloseHandle(hThreadConsumidor);
	CloseHandle(hThreadAgua);

	
	return 0;
}





BOOL verifica(DadosThread* dados) {
	MemPartilhada* mem = dados->memPar;
	unsigned int tab = mem->tabuleiro1;
	int* x = 0;   //dados->posX;
	int* y = 0;   //dados->posY;
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