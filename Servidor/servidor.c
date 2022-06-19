﻿#include "servidor.h"
#include "utils.h"



void alteraSequencia(DadosThread* dados, DadosTabuleiro* tabuleiro) {
	//TALVEZ MUTEX. DEPENDE SE ESTÁ DENTRO DE MUTEX QUANDO A FUNÇÂO É CHAMADA
	
	for (int i = 0; i < 5; i++) {									//passa os tubos uma posição para baixo //tecnicamente, apaga o tubo que se acabou de utilizar
		tabuleiro->sequencia[i] = tabuleiro->sequencia[i + 1];
	}

	if (dados->modoRandom){
		tabuleiro->sequencia[5] = (rand() % 6) + 1;				//numero aleatorio entre 1 e 6
		return;
	}

	//Se não for modo random:
	if (tabuleiro->sequencia[5] >= 6) {							//se for o tubo 6, adiciona o tubo 1
		tabuleiro->sequencia[5] = 1;
		return;
	}
	tabuleiro->sequencia[5] = tabuleiro->sequencia[5] + 1;		//se não, o tubo é o proximo da sequencia

}

DadosTabuleiro* leDePipesOverlapped(DadosThread *dados, int *n, TCHAR buf[MAX]) {
	BOOL isPipe1Pending, isPipe2Pending;

	if (ReadFile(dados->tabuleiro1.pipes.hPipeIn, buf, MAX * sizeof(TCHAR), n, &(dados->tabuleiro1.pipes.overlap)) != FALSE) {					//se TRUE, significa que conseguiu ler imediatamente, sem assinalar o evento de OVERLAP
		_tprintf(TEXT("[SV] LI LOGO PIPE 1: %d %s\n"), *n, buf);
		return &dados->tabuleiro1;																											//retorna-se logo, indicando qual o tabuleiro que enviou mensagem
		
	}

	isPipe1Pending = GetLastError() == ERROR_IO_PENDING;																							//ERROR_IO_PENDING significa que o vai ativar o evento OVERLAP quando receber dados
																																		//Se o erro for diferente, significa que houve outro erro, como por exemplo o pipe não ter ligação do outro lado.

	if (isPipe1Pending)
	{
		_tprintf(TEXT("pipe 1 IO PENFDING\n"));
	}
	else {
		_tprintf(TEXT("NOT IO PENFDING 1 !!!\n"));
	}

	if (ReadFile(dados->tabuleiro2.pipes.hPipeIn, buf, MAX * sizeof(TCHAR), n, &(dados->tabuleiro2.pipes.overlap)) != FALSE) {					//repete-se para o pipe do outro tabuleiro
		_tprintf(TEXT("[SV] LI LOGO PIPE 2: %d %s\n"), *n, buf);

		return &dados->tabuleiro2;
	}

	isPipe2Pending = GetLastError() == ERROR_IO_PENDING;
	if (isPipe2Pending)
	{
		_tprintf(TEXT("pipe 2 IO PENFDING\n"));
	}
	else {
		_tprintf(TEXT("NOT IO PENFDING 2 !!!\n"));
	}

	if (!isPipe1Pending && !isPipe2Pending)
	{
		//_tprintf(TEXT("NONE ARE PENDING!!!\n"));
		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);
		return NULL;
	}
	_tprintf(_T("\n\n%p\n\n"), dados->tabuleiro1.pipes.hPipeIn);

	_tprintf(TEXT("[SV] VOU ESPERAR AGORA\n"));
	WaitForSingleObject(dados->tabuleiro1.pipes.overlap.hEvent, 2000);																//espera pelo evento de overlap. Não faz utilizar o OVERLAPPED da tabuleiro1 ou tabuleiro2, uma vez que utilizam o mesmo evento

	if (GetOverlappedResult(dados->tabuleiro1.pipes.hPipeIn, &(dados->tabuleiro1.pipes.overlap), n, FALSE)) {							//se GetOverlappedResult retornar TRUE para o pipe indicado, significa que este pipe recebeu mensagem
		_tprintf(TEXT("[SV] Recebi de pipe1 overlapped: %d %s\n"), *n, buf);																	//reset ao evento para a proxima iteração
		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);																						//retorna indicando qual o tabuleiro que enviou mensagem
		return &(dados->tabuleiro1);
	
	}
	else if(GetOverlappedResult(dados->tabuleiro2.pipes.hPipeIn, &(dados->tabuleiro2.pipes.overlap), n, FALSE)) {						//caso contrário, só pode ter sido o outro pipe a receber mensagem. Mas convém testar com if pelo sim pelo não
		_tprintf(TEXT("[SV] Recebi de pipe2 overlapped: %d %s\n"), *n, buf);
		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);
		return &dados->tabuleiro2;
	}
	else {
		_tprintf(TEXT("[SV] \n\nerror: %d\n\n"), GetLastError());																	//reset ao evento para a proxima iteração

		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);																	//tecnicamente nunca deve chegar aqui. se chegar é porque houve algum erro
		_tprintf(TEXT("[SV] GetOverlappedResult nao devolveu nenhum resultado esperado\n"));
		return NULL;
	}
}

DWORD WINAPI ThreadLer(LPVOID param) {
	//fica sempre á escuta de ler coisas vindas do named pipe
	//escreve diretamente no ecrã
	DadosThread* dados = (DadosThread*)param;
	TCHAR buf[MAX], ** arrayComandos = NULL;
	DWORD n;
	int multi=0;
	unsigned int nrArgs = 0;
	const TCHAR delim[2] = _T(" ");
	OVERLAPPED o1 = { 0 };
	OVERLAPPED o2 = { 0 };


	//_tprintf(TEXT("[SV] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), NOME_PIPE_CLIENTE);

	//espera que exista um named pipe para ler do mesmo
	//bloqueia aqui
	/*
	if (!WaitNamedPipe(NOME_PIPE_CLIENTE, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}
	*/

	//_tprintf(TEXT("[SV] Ligação ao pipe do cliente... (CreateFile)\n"));

	dados->tabuleiro1.pipes.overlap = o1;
	dados->tabuleiro2.pipes.overlap = o2;

	dados->tabuleiro1.pipes.overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_OVERLAP);
	if (dados->tabuleiro1.pipes.overlap.hEvent == NULL) {
		_tprintf(TEXT("[ERRO] Criar Event! (CreateEvent)"));
		exit(-1);
	}
	dados->tabuleiro2.pipes.overlap.hEvent = dados->tabuleiro1.pipes.overlap.hEvent;

	_tprintf(TEXT("[SV] Liguei-me...\n"));
	DadosTabuleiro* sourceTabuleiro;

	while (dados->terminar == 0) {
		TCHAR a[MAX];
		//le as mensagens enviadas pelo cliente
		dados->tabuleiro1.pipes.overlap.Internal = 0;
		dados->tabuleiro2.pipes.overlap.Internal = 0;
		
		sourceTabuleiro = leDePipesOverlapped(dados, &n, &buf);
		
		if (!n){
			_tprintf(TEXT("[SV] n == 0... (ReadFile)\n"));
			//continue;
		}

		if (sourceTabuleiro == NULL) {
			_tprintf(TEXT("[SV] sourceTabuleiro NULL... %d (ReadFile)\n"), n);
			Sleep(1000);
			continue;
		}

		buf[n / sizeof(TCHAR) - 1] = '\0';

		_tprintf(TEXT("[SV] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);

		arrayComandos = divideString(buf, delim, &nrArgs);			//divisão da string para um array com o comando e args
		/*
		#define LCLICK _T("LCLICK")-
		#define RCLICK _T("RCLICK")-
		#define HOVER _T("HOVER")
		#define RETOMAHOVER _T("RETOMAHOVER")
		#define SAIRCLI _T("SAIRCLI")
		#define JOGOSINGLEP _T("JOGOSINGLEP")
		#define JOGOMULTIP _T("JOGOMULTIP"
		*/
		if (_tcscmp(arrayComandos[0], LCLICK) == 0 || _tcscmp(arrayComandos[0], RCLICK) == 0) {
			if (nrArgs >= 2) {	//x,y,tipopeça
				unsigned int x, y, t;
				if (_tcscmp(arrayComandos[1], _T("0")) != 0) {		//verifica se valor não é igual a '0' pois atoi devolve 0 quando é erro
					x = _tstoi(arrayComandos[1]);
					if (x == 0) {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
				}
				else
					x = 0;
				if (_tcscmp(arrayComandos[2], _T("0")) != 0) {		//verifica se valor não é igual a '0' pois atoi devolve 0 quando é erro
					y = _tstoi(arrayComandos[2]);
					if (y == 0) {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
				}
				else {
					y = 0;
				}

				if (_tcscmp(arrayComandos[0], LCLICK) == 0) {
					//t = dados.sequencia[0];
					t = 2;
				}
				else {
					t = 0;
				}

				WaitForSingleObject(dados->hMutexTabuleiro,INFINITE);
				BOOL colocou = verificaColocarPeca(dados, x, y, t, sourceTabuleiro);
				ReleaseMutex(dados->hMutexTabuleiro);

				
				if (colocou == TRUE) {
					_stprintf_s(a, MAX, _T("PECA %d %d %d\n"), x, y, t);
					escreveNamedPipe(dados,a, sourceTabuleiro);
					//sequencia atualiza
				}
				else {
					if (_tcscmp(arrayComandos[0], LCLICK) == 0) {
						_stprintf_s(a, MAX, _T("INFO NaoFoiPossivelColocarPeca\n"));
						escreveNamedPipe(dados, a, sourceTabuleiro);
					}
					else {
						_stprintf_s(a, MAX, _T("INFO NaoFoiPossivelEliminarPeca\n"));
						escreveNamedPipe(dados, a, sourceTabuleiro);
					}
				}
			}
		}else if (_tcscmp(arrayComandos[0], HOVER) == 0) {
			//if cliente1
				//if numero de paragens cliente1 <=3
					SuspendThread(dados->tabuleiro1.hThreadAgua);
					_stprintf_s(a, MAX, _T("INFO AguaParada\n"));
					escreveNamedPipe(dados, a, sourceTabuleiro);
				//else 
					//_stprintf_s(a, MAX, _T("INFO JáUtilizouTodasAsSuasPausas\n"));
					//escreveNamedPipe(dados, a);
			//if cliente2
				//SuspendThread(dados.tabuleiro2.hThreadAgua);
		}else if (_tcscmp(arrayComandos[0], RETOMAHOVER) == 0) {
			ResumeThread(dados->tabuleiro1.hThreadAgua);
			_stprintf_s(a, MAX, _T("INFO RetomaAgua\n"));
			escreveNamedPipe(dados, a, sourceTabuleiro);
		}else if (_tcscmp(arrayComandos[0], JOGOSINGLEP) == 0) {
			//iniciar jogo
			//if jogo ainda não se encontra em curso
				//if cliente1
					ResumeThread(dados->tabuleiro1.hThreadAgua);
					_stprintf_s(a, MAX, _T("INICIA %d\n"),  dados->tempoInicioAgua);
					escreveNamedPipe(dados, a, sourceTabuleiro);
				//else
					//ResumeThread(dados->tabuleiro1.hThreadAgua);
			//else jogo já se encontra em curso
				//_stprintf_s(a, MAX, _T("INFO JogoEmCurso\n"));
				//escreveNamedPipe(dados, a);
		}else if (_tcscmp(arrayComandos[0], JOGOMULTIP) == 0) {
			// variavel com a quantidade de jogadores a querer jogar multiplayer
			if (multi > 0) {	//existe um jogador em espera
				//iniciar jogo multiplayer...
				//ResumeThread(dados.tabuleiro1.hThreadAgua);
				//_stprintf_s(a, MAX, _T("JOGOMULTIP INICIA\n"));
				//escreveNamedPipe(dados, a);
			}
			else {
				//verificar se cliente2 está ativo
				//if (dados->tabuleiro2.jogadorAtivo == FALSE)
				//	  esperar que um cliente se ligue
				//    _stprintf_s(a, MAX, _T("JOGOMULTIP ESPERA\n"));
				//	  escreveNamedPipe(dados, a);
				//else
				//	  mandar informação de que existe um jogador a tentar jogar multiplayer

			}


		}
		else if (_tcscmp(arrayComandos[0], JOGOMULTIPCANCEL) == 0) {
			multi--;
		}
		else if (_tcscmp(arrayComandos[0], SAIRCLI) == 0) {
			//verificar se jogo ainda se encontra a correr
			//se for multiplayer -> informar ao outro cliente que ganhou
			// 
			//eliminar info cli
			//desconectar named pipes
			DisconnectNamedPipe(dados->tabuleiro1.pipes.hPipeIn);
			DisconnectNamedPipe(dados->tabuleiro2.pipes.hPipeIn);


		}


		//sistema de mensagens

		/*para->água parada
		pi-> impossivel colocar peça no local indicado
		pc-> peça colocada com sucesso
		perdeu -> perdeu
		ganhou -> ganhou
		barr -> barreira adicionada
		...
		*/


		//_tcscpy_s(mensagem, MAX, buf);

	}
	return 0;

}


BOOL escreveNamedPipe(DadosThread* dados, TCHAR* a, DadosTabuleiro* dadosTabuleiro) {
	WaitForSingleObject(dados->hMutexNamedPipe, INFINITE);
	dados->hPipeOut = dadosTabuleiro->pipes.hPipeOut;
	_tcscpy_s(dados->mensagem, MAX, a);
	ReleaseMutex(dados->hMutexNamedPipe);
	SetEvent(dados->hEventoNamedPipe);
}

BOOL verificaColocarPeca(DadosThread* dados, int x, int y, int t, DadosTabuleiro* dadosTabuleiro) {		//adicionar depois qual o tabuleiro a verificar (int tab)
	//verificar se celula é água ou verificar se célula é o fim jogo ou barr
	if (x >= dados->memPar->tamX || y >= dados->memPar->tamY)
	{
		return FALSE;
	}
	
	int tab = 1;
	if (tab == 1) {	//tabuleiro 1
		if (dados->posfX == x && dados->posfY == y)
			return FALSE;
		if ((*dados->tabuleiro1.tabuleiro)[x][y] < 0 || (*dados->tabuleiro1.tabuleiro)[x][y] == 7) {
			return FALSE;
		}
		else {
			(*dados->tabuleiro1.tabuleiro)[x][y] = t;
			return TRUE;
		}
	}
	else { //tabuleiro 2
		if ((*dados->tabuleiro2.tabuleiro)[x][y] < 0 || (dados->posfX == x && dados->posfY == y) || (*dados->tabuleiro2.tabuleiro)[x][y] == 7) {
			return FALSE;
		}
		else {
			(*dados->tabuleiro2.tabuleiro)[x][y] = t;
			return TRUE;
		}
	}

	return FALSE;
}

DWORD WINAPI ThreadEscrever(LPVOID param) {								//thread escritura de informações para o cliente através do named pipe
	//funcionamento -> evento ativa quando texto variável de "escrita" é alterada
	DadosThread* dados = (DadosThread*)param;
	DWORD n;
	TCHAR buf[MAX]=_T("yodvbasucansdo");
	int i;
	//_tcscpy_s(buf, MAX, _T("aygfhilvanvoadf\n"));
	
	//alteras str -> acionas evento -> envia info (str) pela thread

	do {
		//ficar bloqueado à espera de um evento 
		WaitForSingleObject(dados->hEventoNamedPipe, INFINITE);

		_tcscpy_s(dados->mensagem, MAX, buf);


		//escreve no named pipe
		WaitForSingleObject(dados->hMutexNamedPipe, INFINITE);
		if (!WriteFile(dados->hPipeOut, dados->mensagem, _tcslen(dados->mensagem) * sizeof(TCHAR), &n, NULL))
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		else
			_tprintf(TEXT("[SV] Enviei %d bytes ao cliente ... (WriteFile)\n"), n);
		//libertamos o mutex
		dados->hPipeOut = NULL;
		ReleaseMutex(dados->hMutexNamedPipe);

		ResetEvent(dados->hEventoNamedPipe);

	} while (dados->terminar==0);
	dados->terminar = 1;


	return 0;
}

DWORD WINAPI ThreadConsumidor(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	CelulaBuffer celula;
	TCHAR comando[MAX], ** arrayComandos = NULL;
	const TCHAR delim[2] = _T(" ");
	unsigned int nrArgs = 0;
	HANDLE hSemLeituraWait[2] = {dados->hSemLeitura, dados->hEventTerminar};


	while (dados->terminar == 0) {	//termina quando dados.terminar != 0

		if(WaitForMultipleObjects(2, hSemLeituraWait, FALSE, INFINITE) == WAIT_OBJECT_0 + 1){		//fica à espera que o semaforo fique assinalado
			return 0;
		}
			
		WaitForSingleObject(dados->hMutexBufferCircular, INFINITE);									//fica à espera do unlock do mutex


		CopyMemory(&celula, &dados->memPar->buffer[dados->memPar->posL], sizeof(CelulaBuffer));		//copia da memória os próximos dados
		dados->memPar->posL++; 
		if (dados->memPar->posL == TAM_BUFFER) 
			dados->memPar->posL = 0; 

		_tcscpy_s(comando, MAX, celula.comando);
		_tprintf(_T("P%d consumiu %s.\n"), celula.id, celula.comando);

		free(arrayComandos);	//libertar a memoria de args

		arrayComandos = divideString(comando, delim, &nrArgs);			//divisão da string para um array com o comando e args
		
		if (arrayComandos != NULL ) {
			_tprintf(_T("\ncomando: %s.\n"), arrayComandos[0]);
			for (int i = 0; i < nrArgs; i++)
			{
				_tprintf(_T("arg[%d]: %s.\n"), i, arrayComandos[i]);
			}
			if (_tcscmp(arrayComandos[0], PFAGUA) == 0)			//comando para fluxo agua por determinado tempo
			{
				_tprintf(_T("pfaguaaaaaa\n"));
				if (!dados->iniciado)
				{
					_tprintf(TEXT("Jogo não está em curso\n"));
				}
				else {
					if (nrArgs > 1) {
						int tempo = _tstoi(arrayComandos[1]);
						if (tempo != 0 && tempo < 25 && tempo > 1) {	//se for válido ou de 2s a 25s
							dados->parafluxo = tempo;
							WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
							_tcscpy_s(dados->memPar->estado, MAX, _T("Fluxo de água em pausa"));
							ReleaseMutex(dados->hMutexTabuleiro);
							SetEvent(dados->hEventUpdateTabuleiro);
						}
						else
							_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
					else {
						_tprintf(_T("Numero de argumentos insuficientes\n"));
					}

				}							}
			else if (_tcscmp(arrayComandos[0], INICIAR) == 0)	//comando para fluxo agua por determinado tempo
			{
				if (dados->iniciado) {
					_tprintf(_T("Jogo já está em curso\n"));
				}
				else {
					if(*dados->tabuleiro1.jogadorAtivo)
						ResumeThread(dados->tabuleiro1.hThreadAgua);
					if (*dados->tabuleiro2.jogadorAtivo)
						ResumeThread(dados->tabuleiro2.hThreadAgua);
					WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
					_tcscpy_s(dados->memPar->estado, MAX, _T("Jogo iniciado"));
					ReleaseMutex(dados->hMutexTabuleiro);
					dados->iniciado = TRUE;
				}
			}
			else if (_tcscmp(arrayComandos[0], BARR) == 0)		//comando adicionar barreira
			{
				int x = 0,y = 0;
				_tprintf(_T("barrrrrr\n"));
				if (nrArgs > 2) {
					if (_tcscmp(arrayComandos[1], _T("0") ) != 0) {		//verifica se valor é igual a '0' pois atoi devolve 0 quando é erro
						x = _tstoi(arrayComandos[1]);
						if (x == 0) {
							_tprintf(_T("Valor passado como argumento não é aceite\n"));
						}
					}
					if (_tcscmp(arrayComandos[2], _T("0")) != 0) {	//verifica se valor é igual a '0' pois atoi devolve 0 quando é erro
						y = _tstoi(arrayComandos[2]);
						if (y == 0) {
							_tprintf(_T("Valor passado como argumento não é aceite\n"));
						}
					}
					WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);	//bloqueia à espera do unlock do mutex
					
					if (x < dados->memPar->tamX && x >= 0 && y < dados->memPar->tamY && y >= 0) {		//se for válido ou de 2s a 25s
						if (*dados->tabuleiro1.jogadorAtivo) {
							if ((*dados->tabuleiro1.tabuleiro)[x][y] == 0) {		//espaço livre
								(*dados->tabuleiro1.tabuleiro)[x][y] = 7;
								_tcscpy_s(dados->memPar->estado, MAX, _T("Barreira adicionada"));
							}
							else {
								_tcscpy_s(dados->memPar->estado, MAX, _T("Barreira não pôde ser adicionada"));
								_tprintf(_T("Não foi possivel adicionar a barreira\n"));
							}
						}
						if (*dados->tabuleiro2.jogadorAtivo) {
							if ((*dados->tabuleiro2.tabuleiro)[x][y] == 0) {		//espaço livre
								(*dados->tabuleiro2.tabuleiro)[x][y] = 7;
								_tcscpy_s(dados->memPar->estado, MAX, _T("Barreira adicionada"));
							}
							else {
								_tcscpy_s(dados->memPar->estado, MAX, _T("Barreira não pôde ser adicionada"));
								_tprintf(_T("Não foi possivel adicionar a barreira\n"));
							}
						}
						SetEvent(dados->hEventUpdateTabuleiro);			//assinala evento de atualização do tabuleiro

					}
					else {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
					ReleaseMutex(dados->hMutexTabuleiro);
				}
				else {
					_tprintf(_T("Numero de argumentos insuficientes\n"));
				}
			}
			else if (_tcscmp(arrayComandos[0], MODO) == 0)							//comando alterar modo sequencia tubos
			{
				_tprintf(_T("modooooooo\n"));
				//if(dados->modo == 1)dados->modo = 0; else dados->modo = 1;		//modo = 1 -> aleatorio, modo = 0 -> sequecia predefinida
			}
			else //comando nao encontrado
			{
				_tprintf(_T("Comando desconhecido!\n"));
				// comando desconhecido
			}
		}
		ReleaseMutex(dados->hMutexBufferCircular);			//liberta mutex
		ReleaseSemaphore(dados->hSemEscrita, 1, NULL);		//liberta semaforo
	}//while
	return 0;
}

void carregaMapaPreDefinido(DadosThread* dados, int tabuleiro[20][20]) {
	tabuleiro[0][2] = -1;
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
	dados->posfX = 9;
	dados->posfY = 5;
}

BOOL definirInicioFim(DadosThread* dados) {
	srand(time(0));
	
	int parede = (rand() % (4));						//define em que parede vai estar :  0 - cima // 1 - direita //2 - baixo // 3 - esquerda
	int min = 0, max, tipoTubo, posInicio, posFim;

	switch (parede){
	case 0:												//se estiver em cima ou em baixo, o limite máximo da sua posição é tamH, e o tubo inicial é vertical
	case 2:
		max = dados->memPar->tamX;
		tipoTubo = 2;
		break;
	case 1:												//se estiver à direita ou esquerda, o limite máximo da sua posição é tamV, e o tubo inicial é horizontal
	case 3:
		max = dados->memPar->tamY;
		tipoTubo = 1;
		break;
	default:											//se o valor aleatorio não for um dos esperados, retorna FALSE (apenas se ocorrer algum erro)
		return FALSE;
	}

	posInicio = (rand() % max);							//posição a ocupar, na parede 
	
	if (max%2 == 0){								//se max par
		if (posInicio >= max / 2) {						//e posInicio maior que metade, posFim tem de estar na primeira metade da parede
			max = max / 2;
		}
		else {											//e posInicio menor que metade, posFim tem de estar na segunda metade da parede
			min = max / 2;
		}
	}
	else {											//se max impar
		if (posInicio > max / 2) {						//e posInicio maior que metade inteira, posFim tem de estar na primeira metade da parede
			max = (max / 2) + 1;
		}
		else if (posInicio < max / 2) {					//e posInicio menor que metade inteira, posFim tem de estar na primeira metade da parede
			min = max / 2;						
		}												//se max impar e posInicio igual à metade inteira, posInicio está na posição do meio, logo posFim pode estar em qualquer posição da parede
	}

	posFim = ((rand() % (max - min))+min);
	int ultimaPos;

	switch (parede) {							
	case 0:																				//Parede de cima
		//dados->tabuleiro1.tabuleiro[posInicio][0] = tipoTubo * -1;						//mete tubo correto na posição de inicio
		(*dados->tabuleiro2.tabuleiro)[posInicio][0] = tipoTubo * -1;						//mete tubo com água correto na posição de inicio

		//dados->tabuleiro1.posX = posInicio;												//posição de agua ativa = posição de inicio
		//dados->tabuleiro1.posY = 0;
		dados->tabuleiro2.posX = posInicio;													//posição de agua ativa = posição de inicio
		dados->tabuleiro2.posY = 0;
		
		//dados->tabuleiro1.tabuleiro[posFim][dados->memPar->tamY - 1] = tipoTubo;			//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		(*dados->tabuleiro2.tabuleiro)[posFim][dados->memPar->tamY - 1] = tipoTubo;			//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		
		//dados->tabuleiro1.dirAgua = BAIXO;
		dados->tabuleiro2.dirAgua = BAIXO;

		//dados->posfX = posFim;
		//dados->posfY = dados->memPar->tamY - 1;

		break;
	case 1:
		ultimaPos = dados->memPar->tamX - 1;											//pareda lado direito
		//dados->tabuleiro1.tabuleiro[ultimaPos][posInicio] = tipoTubo * -1;
		(*dados->tabuleiro2.tabuleiro)[ultimaPos][posInicio] = tipoTubo * -1;

		//dados->tabuleiro1.posX = ultimaPos;
		//dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = ultimaPos;
		dados->tabuleiro2.posY = posInicio;
		
		//dados->tabuleiro1.tabuleiro[0][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[0][posFim] = tipoTubo;

		//dados->tabuleiro1.dirAgua = ESQUERDA;
		dados->tabuleiro2.dirAgua = ESQUERDA;

		//dados->posfX = 0;
		//dados->posfY = posFim;

		break;
	case 2:
		ultimaPos = dados->memPar->tamY - 1;											//parede baixo
		//dados->tabuleiro1.tabuleiro[posInicio][ultimaPos] = tipoTubo * -1;
		(*dados->tabuleiro2.tabuleiro)[posInicio][ultimaPos] = tipoTubo * -1;

		//dados->tabuleiro1.posX = posInicio;
		//dados->tabuleiro1.posY = ultimaPos;
		dados->tabuleiro2.posX = posInicio;
		dados->tabuleiro2.posY = ultimaPos;

		//dados->tabuleiro1.tabuleiro[posFim][0] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[posFim][0] = tipoTubo;

		//dados->tabuleiro1.dirAgua = CIMA;
		dados->tabuleiro2.dirAgua = CIMA;

		//dados->posfX = posFim;
		//dados->posfY = 0;
		
		break;
	case 3:
		//dados->tabuleiro1.tabuleiro[0][posInicio] = tipoTubo * -1;					//parede lado esquerdo
		(*dados->tabuleiro2.tabuleiro)[0][posInicio] = tipoTubo * -1;

		//dados->tabuleiro1.posX = 0;
		//dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = 0;
		dados->tabuleiro2.posY = posInicio;

		//dados->tabuleiro1.tabuleiro[dados->memPar->tamX - 1][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[dados->memPar->tamX - 1][posFim] = tipoTubo;

		//dados->tabuleiro1.dirAgua = DIREITA;
		dados->tabuleiro2.dirAgua = DIREITA;

		//dados->posfX = dados->memPar->tamX - 1;
		//dados->posfY = posFim;
		
		break;
	default:
		return FALSE;
	} 
	return TRUE;
}


BOOL fluirAgua(DadosThread* dados, DadosTabuleiro* dadosTabuleiro) {
	switch(dadosTabuleiro->dirAgua){
	case CIMA:
		if (dadosTabuleiro->posY - 1 < 0) {
			return FALSE;
		}
		switch ((*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY - 1]) {
		case 2:				// peça ┃
			_tprintf(_T("encontrei peça ┃, direcao: CIMA\n"));
			break;
		case 3:				// peça ┏
			dadosTabuleiro->dirAgua = DIREITA;
			_tprintf(_T("encontrei peça ┏, direcao: DIREITA\n"));
			break;
		case 4:				// peça ┓
			dadosTabuleiro->dirAgua = ESQUERDA;
			_tprintf(_T("encontrei peça ┓, direcao: ESQUERDA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dadosTabuleiro->posY--;															//avança a posição ativa da água na direção de fluxo
		(*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case DIREITA:
		if (dadosTabuleiro->posX + 1 >= dados->memPar->tamX) {
			return FALSE;
		}
		switch ((*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX+1][dadosTabuleiro->posY]) {
		case 1:				// peça ━
			_tprintf(_T("encontrei peça ━, direcao: DIREITA\n"));
			break;
		case 4:				// peça ┓
			dadosTabuleiro->dirAgua = BAIXO;
			_tprintf(_T("encontrei peça ┓, direcao: BAIXO\n"));
			break;
		case 5:				// peça ┛
			dadosTabuleiro->dirAgua = CIMA;
			_tprintf(_T("encontrei peça ┛, direcao: CIMA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dadosTabuleiro->posX++;															//avança a posição ativa da água na direção de fluxo
		(*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case BAIXO:
		if (dadosTabuleiro->posY + 1 >= dados->memPar->tamY) {
			return FALSE;
		}
		switch ((*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY + 1]) {
		case 2:				// peça ┃
			_tprintf(_T("encontrei peça ┃, direcao: BAIXO\n"));
			break;
		case 5:				// peça ┛
			dadosTabuleiro->dirAgua = ESQUERDA;
			_tprintf(_T("encontrei peça ┛, direcao: ESQUERDA\n"));
			break;
		case 6:				// peça ┗
			dadosTabuleiro->dirAgua = DIREITA;
			_tprintf(_T("encontrei peça ┗, direcao: DIREITA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dadosTabuleiro->posY++;															//avança a posição ativa da água na direção de fluxo
		(*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	case ESQUERDA:
		if (dadosTabuleiro->posX - 1 < 0) {
			return FALSE;
		}
		switch ((*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX - 1][dadosTabuleiro->posY]) {
		case 1:				// peça ━
			_tprintf(_T("encontrei peça ━, direcao: ESQUERDA\n"));
			break;
		case 3:				// peça ┏
			dadosTabuleiro->dirAgua = BAIXO;
			_tprintf(_T("encontrei peça ┏, direcao: BAIXO\n"));
			break;
		case 6:				// peça ┗
			dadosTabuleiro->dirAgua = CIMA;
			_tprintf(_T("encontrei peça ┗, direcao: CIMA\n"));
			break;
		default:			//encontra peça inutilizável ou vazio ou barreira
			return FALSE;
		}
		dadosTabuleiro->posX--;															//avança a posição ativa da água na direção de fluxo
		(*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY] *= -1;				//coloca a peça ativa como tendo água
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI ThreadAgua(LPVOID param) {
	DadosThreadAgua* paramStruct = (DadosThreadAgua*)param;
	DadosThread* dados = paramStruct->dados;
	DadosTabuleiro* dadosTabuleiro;
	switch (paramStruct->numTabuleiro){
	case 1:
		dadosTabuleiro = &(dados->tabuleiro1);
		break;
	case 2:
		dadosTabuleiro = &(dados->tabuleiro2);
		break;
	default:
		return;
	}
	

	SetEvent(dados->hEventUpdateTabuleiro);														//assinala evento de atualização do tabuleiro
	_tprintf(_T("(ThreadAgua) Sleep %d"), dados->tempoInicioAgua * 1000);			
	Sleep(dados->tempoInicioAgua * 1000);														//jogo incia após "tempoInicioAgua"

		while (!dados->terminar) {
			if (dados->parafluxo == 0) {
				WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
				if (dadosTabuleiro->jogadorAtivo){
					_tprintf(_T("TABULEIRO %d ATIVO"), paramStruct->numTabuleiro);
				}
				if (fluirAgua(dados, dadosTabuleiro)) {															//verifica se a água fluiu com sucesso
					if (dadosTabuleiro->posX == dados->posfX && dadosTabuleiro->posY == dados->posfY){
						_tcscpy_s(dados->memPar->estado, MAX, _T("Ganhou!!!"));					//muda o estado do jogo 
						_tprintf(_T("(ThreadAgua) Ganhou!!!"));
						SetEvent(dados->hEventUpdateTabuleiro);
						ReleaseMutex(dados->hMutexTabuleiro);
						break;
					}
					else {
						_tcscpy_s(dados->memPar->estado, MAX, _T("Água fluiu"));				//muda o estado do jogo 
						_tprintf(_T("(ThreadAgua) Fluir água! Sleep %d"), TIMER_FLUIR * 1000);
					}
					ReleaseMutex(dados->hMutexTabuleiro);
					SetEvent(dados->hEventUpdateTabuleiro);
					Sleep(TIMER_FLUIR * 1000);
				}
				else {
					_tcscpy_s(dados->memPar->estado, MAX, _T("Perdeu!!!"));						//muda o estado do jogo 
					_tprintf(_T("(ThreadAgua) Perdeu!!!"));
					ReleaseMutex(dados->hMutexTabuleiro);
					SetEvent(dados->hEventUpdateTabuleiro);
					_tprintf(_T("(ThreadAgua) FluirAgua -> FALSE"));
					break;
				}
			}
			else {
				Sleep(dados->parafluxo * 1000);													//tempo do fluxo da água
				dados->parafluxo = 0;
			}
		}
	dados->iniciado = FALSE;
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
		_tprintf(_T("Erro: MapViewOfFile (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		return FALSE;
	}

	dados->memPar->nP = 0;
	dados->memPar->posE = 0;
	dados->memPar->posL = 0;
	dados->parafluxo = 0;

	//Comentado por causa do mapa pré-definido da meta 1
	//dados->memPar->tamX = tamH;
	//dados->memPar->tamY = tamV;
		
	dados->memPar->tamX = 10;
	dados->memPar->tamY = 7;

	dados->tabuleiro1.tabuleiro = &(dados->memPar->dadosTabuleiro1.tabuleiro);
	dados->tabuleiro1.jogadorAtivo = &(dados->memPar->dadosTabuleiro1.jogadorAtivo);
	dados->tabuleiro2.tabuleiro = &(dados->memPar->dadosTabuleiro2.tabuleiro);
	dados->tabuleiro2.jogadorAtivo = &(dados->memPar->dadosTabuleiro2.jogadorAtivo);
	
	for (int i = 0; i < tamH; i++) {
		for (int j = 0; j < tamV; j++) {
			(*dados->tabuleiro1.tabuleiro)[i][j] = 0;

			(*dados->tabuleiro2.tabuleiro)[i][j] = 0;		//apenas meta 1. Depois será feito -> tabuleiro2 = tabuleiro1 (cópia de valores)
		}
	}

	*dados->tabuleiro1.jogadorAtivo = FALSE;
	*dados->tabuleiro2.jogadorAtivo = FALSE;


	//	MAPA PRÉ-DEFINIDO
	carregaMapaPreDefinido(dados, dados->tabuleiro1.tabuleiro);
	
	*dados->tabuleiro1.jogadorAtivo = TRUE;

	// Define início e fim
	definirInicioFim(dados);


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

	dados->hEventUpdateTabuleiro = CreateEvent(NULL, TRUE, FALSE, EVENT_TABULEIRO);
	if (dados->hEventUpdateTabuleiro == NULL) {
		_tprintf(_T("Erro: CreateEvent (%d)\n"), GetLastError());
		UnmapViewOfFile(dados->hMapFile);
		CloseHandle(dados->memPar);
		CloseHandle(dados->hMapFile);
		return FALSE;
	}

	dados->hMutexTabuleiro = CreateMutex(NULL, FALSE, MUTEX_TABULEIRO);
	if (dados->hMutexTabuleiro == NULL) {
		_tprintf(_T("Erro: CreateMutex (%d)\n"), GetLastError());
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

	dados->iniciado = FALSE;

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
		if (RegQueryValueEx(hChaveRegistry, nomeValorRegistry, NULL, NULL, &valorNum, &sizeValor) == ERROR_SUCCESS) {			//vai buscar valor registry
			*varGuardar = valorNum;
			return 1;
		}
		else {
			*varGuardar = valorOmissao;
			RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(unsigned int));					//altera valor registry
			return 2;
		}
	}
	else {
		*varGuardar = valorNum;
		RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(unsigned int));						//altera valor registry
		return 3;
	}
}



BOOL initNamedPipes(DadosThread* dados) {
	dados->terminar = 0;
	dados->numClientes = 0;


	dados->hMutexNamedPipe = CreateMutex(NULL, FALSE, MUTEX_NPIPE_SV); //Criação do mutex
	if (dados->hMutexNamedPipe == NULL) {
		_tprintf(TEXT("[Erro] ao criar mutex!\n"));
		return FALSE;
	}

	WaitForSingleObject(dados->hMutexNamedPipe, INFINITE);
	dados->tabuleiro1.pipes.hPipeOut = CreateNamedPipe(NOME_PIPE_SERVIDOR, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 3, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeOut == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe S->C! (CreateNamedPipe)"));
		ReleaseMutex(dados->hMutexNamedPipe);
		CloseHandle(dados->hMutexNamedPipe);
		return FALSE;
	}
	
	dados->tabuleiro2.pipes.hPipeOut = CreateNamedPipe(NOME_PIPE_SERVIDOR, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 3, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeOut == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe S->C! (CreateNamedPipe)"));
		ReleaseMutex(dados->hMutexNamedPipe);
		CloseHandle(dados->hMutexNamedPipe);
		return FALSE;
	}
	ReleaseMutex(dados->hMutexNamedPipe);

	dados->tabuleiro1.pipes.hPipeIn = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 3, MAX * sizeof(TCHAR), MAX * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeIn == NULL) {
		_tprintf(TEXT("[ERRO] Criar instacia do pipe '%s'! (CreateNamedPipe)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}
	_tprintf(_T("\n\n%p\n\n"), dados->tabuleiro1.pipes.hPipeIn);

	dados->tabuleiro2.pipes.hPipeIn = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 3, MAX * sizeof(TCHAR), MAX * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro2.pipes.hPipeIn == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateNamedPipe)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}

	dados->hEventoNamedPipe = CreateEvent(NULL, TRUE, FALSE, EVENT_NAMEDPIPE_SV);
	if (dados->hEventoNamedPipe == NULL) {
		_tprintf(_T("Erro: CreateEvent NamedPipe(%d)\n"), GetLastError());
		CloseHandle(dados->hMutexNamedPipe);
		return FALSE;
	}
}




int _tmain(int argc, LPTSTR argv[]) {
	HKEY hChaveRegistry;
	DWORD respostaRegistry;

	DWORD tamHorizontal, tamVertical;

	HANDLE hThreadConsumidor, hThreadEscrever, hThreadLer;
	DadosThread dados;
	TCHAR comando[MAX];
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
		carregaValorConfig(argv[1], hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal, 3, 20);							//carrega tamanho horizontal tabela
		carregaValorConfig(argv[2], hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical, 3, 20);							//carrega tamanho vertical tabela
		carregaValorConfig(argv[3], hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &(dados.tempoInicioAgua), 5, 45);			//carrega tempo para inicio fluxo agua
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
	DadosThreadAgua dadosAguaTabuleiro1;
	dadosAguaTabuleiro1.dados = &dados;
	dadosAguaTabuleiro1.numTabuleiro = 1;
	dados.tabuleiro1.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosAguaTabuleiro1, CREATE_SUSPENDED, NULL);					//thread criada suspensa até que monitor inicie o jogo

	DadosThreadAgua dadosAguaTabuleiro2;
	dadosAguaTabuleiro2.dados = &dados;
	dadosAguaTabuleiro2.numTabuleiro = 2;
	dados.tabuleiro2.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosAguaTabuleiro2, CREATE_SUSPENDED, NULL);					//thread criada suspensa até que monitor inicie o jogo


	if (!initNamedPipes(&dados)) {
		_tprintf(_T("Erro ao criar named Pipes do Servidor.\n"));
		dados.terminar = 1;
		terminar(&dados,&dados);
		CloseHandle(hThreadConsumidor);
		exit(1);
	}
	
	//thread atendeCliente
	//HANDLE hThreadAClientes = CreateThread(NULL, 0, ThreadAceitaClientes, &dados, 0, NULL);
	//if (hThreadAClientes == NULL) {
	//	_tprintf(TEXT("[Erro] ao criar thread!\n"));
	//	return -1;
	//}

	
	//criacao da thread ESCREVER
	hThreadEscrever = CreateThread(NULL, 0, ThreadEscrever, &dados, 0, NULL);
	if (hThreadEscrever == NULL) {
		_tprintf(TEXT("[Erro] ao criar thread!\n"));
		return -1;
	}

	_tprintf(TEXT("[ESCRITOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"), NOME_PIPE_SERVIDOR);
	dados.hThreadEscrever = hThreadEscrever;


	//criacao da thread LER
	hThreadLer = CreateThread(NULL, 0, ThreadLer, &dados, 0, NULL);
	if (hThreadLer == NULL) {
		_tprintf(TEXT("[Erro] ao criar thread!\n"));
		return -1;
	}
	dados.hThreadLer = hThreadLer;





	if (hThreadConsumidor != NULL && dados.tabuleiro1.hThreadAgua != NULL && dados.tabuleiro2.hThreadAgua != NULL) {
		_tprintf(_T("Escreva 'SAIR' para sair.\n"));
		do {
			fflush(stdin);
			_fgetts(comando, MAX-2, stdin);
			//Retirar \n
			comando[_tcslen(comando) - 1] = '\0';
			//Maiúsculas
			for (int i = 0; i < _tcslen(comando); i++)
				comando[i] = _totupper(comando[i]);
			_tprintf(TEXT("Comando : %s\n"), comando);

			if (_tcscmp(comando, PAUSA) == 0) {
				if (dados.iniciado == TRUE) {
					WaitForSingleObject(dados.hMutexTabuleiro, INFINITE);
					_tcscpy_s(dados.memPar->estado, MAX, _T("Jogo em pausa"));
					_tprintf(TEXT("Jogo em pausa\n"));
					if (dados.tabuleiro1.jogadorAtivo)
						SuspendThread(dados.tabuleiro1.hThreadAgua);
					if (dados.tabuleiro2.jogadorAtivo)
						SuspendThread(dados.tabuleiro2.hThreadAgua);
					
					ReleaseMutex(dados.hMutexTabuleiro);
					SetEvent(dados.hEventUpdateTabuleiro);
				}
				else {
					_tprintf(TEXT("Jogo não está em curso\n"));
				}
			}
			if (_tcscmp(comando, RETOMAR) == 0) {
				if (dados.iniciado == TRUE) {
					WaitForSingleObject(dados.hMutexTabuleiro, INFINITE);
					_tcscpy_s(dados.memPar->estado, MAX, _T("Jogo retomado"));
					ReleaseMutex(dados.hMutexTabuleiro);
					_tprintf(TEXT("Jogo retomado\n"));
					if (dados.tabuleiro1.jogadorAtivo)
						ResumeThread(dados.tabuleiro1.hThreadAgua);
					if (dados.tabuleiro2.jogadorAtivo)
						ResumeThread(dados.tabuleiro2.hThreadAgua);
					SetEvent(dados.hEventUpdateTabuleiro);
				}
				else {
					_tprintf(TEXT("Jogo não está em curso\n"));
				}
			}
		} while (_tcscmp(comando, SAIR) != 0);
		dados.terminar = 1;
		SetEvent(dados.hEventTerminar);
		
		HANDLE hThreadsWait[] = { hThreadConsumidor, dados.tabuleiro1.hThreadAgua, dados.tabuleiro2.hThreadAgua};

		WaitForMultipleObjects(2, hThreadsWait, TRUE, 100);
	}
	CloseHandle(hThreadConsumidor);
	terminar(&dados);
	
	return 0;
}


int terminar(DadosThread* dados) {
	dados->tabuleiro1.tabuleiro = NULL;
	dados->tabuleiro2.tabuleiro = NULL;
	UnmapViewOfFile(dados->memPar);
	//CloseHandles...
	CloseHandle(dados->hMutexBufferCircular);
	CloseHandle(dados->hSemEscrita);
	CloseHandle(dados->hSemLeitura);
	CloseHandle(dados->hMapFile);
	CloseHandle(dados->tabuleiro1.hThreadAgua);
	CloseHandle(dados->tabuleiro2.hThreadAgua);
	CloseHandle(dados->hEventUpdateTabuleiro);
	CloseHandle(dados->hMutexTabuleiro);
	CloseHandle(dados->hEventTerminar);
	if (!DisconnectNamedPipe(dados->tabuleiro1.pipes.hPipeOut)) {
		_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
		exit(-1);
	}
	if (!DisconnectNamedPipe(dados->tabuleiro2.pipes.hPipeOut)) {
		_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
		exit(-1);
	}
}