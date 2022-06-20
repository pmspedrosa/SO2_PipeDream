#include "servidor.h"
#include "utils.h"

void prepararInicioDeJogo(DadosThread* dados) {
	WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
	for (int i = 0; i < dados->memPar->tamX; i++) {
		for (int j = 0; j < dados->memPar->tamY; j++) {
			(*dados->tabuleiro1.tabuleiro)[i][j] = 0;

			(*dados->tabuleiro2.tabuleiro)[i][j] = 0;		//apenas meta 1. Depois será feito -> tabuleiro2 = tabuleiro1 (cópia de valores)
		}
	}
	definirInicioFim(dados);
	ReleaseMutex(dados->hMutexTabuleiro);
	dados->tabuleiro1.sequencia[5] = 1;
	dados->tabuleiro2.sequencia[5] = 1;
	for (int i = 0; i < 5; i++){
		alteraSequencia(dados, &(dados->tabuleiro1));
		alteraSequencia(dados, &(dados->tabuleiro2));
	}
	dados->tabuleiro1.numParagensDisponiveis = NUM_PARAGENS;
	dados->tabuleiro2.numParagensDisponiveis = NUM_PARAGENS;
}

void alteraSequencia(DadosThread* dados, DadosTabuleiro* tabuleiro) {
	//TALVEZ MUTEX. DEPENDE SE ESTÁ DENTRO DE MUTEX QUANDO A FUNÇÂO É CHAMADA
	TCHAR a[MAX];
	for (int i = 0; i < 5; i++) {									//passa os tubos uma posição para baixo //tecnicamente, apaga o tubo que se acabou de utilizar
		tabuleiro->sequencia[i] = tabuleiro->sequencia[i + 1];
	}

	if (dados->modoRandom){
		tabuleiro->sequencia[5] = (rand() % 6) + 1;				//numero aleatorio entre 1 e 6
	}else {
		//Se não for modo random:
		if (tabuleiro->sequencia[5 - 1] >= 6) {							//se for o tubo 6, adiciona o tubo 1
			tabuleiro->sequencia[5] = 1;
		}
		else {
			tabuleiro->sequencia[5] = tabuleiro->sequencia[5 - 1] + 1;		//se não, o tubo é o proximo da sequencia
		}
	}
	_stprintf_s(a, MAX, _T("SEQ %d %d %d %d %d %d\n"), tabuleiro->sequencia[0], tabuleiro->sequencia[1], tabuleiro->sequencia[2], tabuleiro->sequencia[3], tabuleiro->sequencia[4], tabuleiro->sequencia[5]);
	escreveNamedPipe(dados, a, tabuleiro);
	
}

DadosTabuleiro* leDePipesOverlapped(DadosThread *dados, int *n, TCHAR buf[MAX]) {
	BOOL isPipe1Pending, isPipe2Pending;

	if (ReadFile(dados->tabuleiro1.pipes.hPipeIn, buf, MAX * sizeof(TCHAR), n, &(dados->tabuleiro1.pipes.overlap)) != FALSE) {					//se TRUE, significa que conseguiu ler imediatamente, sem assinalar o evento de OVERLAP
		_tprintf(TEXT("[SV] LI LOGO PIPE 1: %d %s\n"), *n, buf);
		return &dados->tabuleiro1;																											//retorna-se logo, indicando qual o tabuleiro que enviou mensagem
		
	}

	isPipe1Pending = GetLastError() == ERROR_IO_PENDING;																							//ERROR_IO_PENDING significa que o vai ativar o evento OVERLAP quando receber dados
	(* dados->tabuleiro1.jogadorAtivo) = isPipe1Pending;


	if (ReadFile(dados->tabuleiro2.pipes.hPipeIn, buf, MAX * sizeof(TCHAR), n, &(dados->tabuleiro2.pipes.overlap)) != FALSE) {					//repete-se para o pipe do outro tabuleiro
		_tprintf(TEXT("[SV] LI LOGO PIPE 2: %d %s\n"), *n, buf);

		return &dados->tabuleiro2;
	}

	isPipe2Pending = GetLastError() == ERROR_IO_PENDING;
	(* dados->tabuleiro2.jogadorAtivo) = isPipe2Pending;


	if (!isPipe1Pending && !isPipe2Pending)
	{
		//_tprintf(TEXT("NONE ARE PENDING!!!\n"));
		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);
		return NULL;
	}

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
		ResetEvent(dados->tabuleiro1.pipes.overlap.hEvent);																	//tecnicamente nunca deve chegar aqui. se chegar é porque houve algum erro
		_tprintf(TEXT("[SV] GetOverlappedResult nao devolveu nenhum resultado esperado\n"));
		return NULL;
	}
}


/*****************************************/
/***************ThreadLer*****************/
/*****************************************/
DWORD WINAPI ThreadLer(LPVOID param) {
	DadosThread* dados = (DadosThread*)param;
	TCHAR buf[MAX], ** arrayComandos = NULL;
	DWORD n;
	int multi=0;
	unsigned int nrArgs = 0;
	const TCHAR delim[2] = _T(" ");
	OVERLAPPED o1 = { 0 };
	OVERLAPPED o2 = { 0 };

	dados->tabuleiro1.pipes.overlap = o1;
	dados->tabuleiro2.pipes.overlap = o2;

	dados->tabuleiro1.pipes.overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_OVERLAP);		//criar evento overlapped
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

		if (sourceTabuleiro == NULL && !n) {
			continue;
		}

		buf[n / sizeof(TCHAR) - 1] = '\0';

		_tprintf(TEXT("[SV] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);

		arrayComandos = divideString(buf, delim, &nrArgs);			//divisão da string para um array com o comando e args
		/* 
		#define LCLICK _T("LCLICK")
		#define RCLICK _T("RCLICK")
		#define HOVER _T("HOVER")
		#define RETOMAHOVER _T("RETOMAHOVER")
		#define SAIRCLI _T("SAIRCLI")
		#define JOGOSINGLEP _T("JOGOSINGLEP")
		#define JOGOMULTIP _T("JOGOMULTIP"
		#define JOGOMULTIPCANCEL _T("JOGOMULTIPCANCEL")
		#define INICIAJOGO _T("INICIAJOGO")
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

				if (_tcscmp(arrayComandos[0], LCLICK) == 0) {		//se LCLICK, adiciona proxima peça da sequencia
					t = sourceTabuleiro->sequencia[0];
				}
				else {												//se RCLICK, remove a peça selecionada
					t = 0;
				}

				WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
				BOOL colocou = verificaColocarPeca(dados, x, y, t, sourceTabuleiro);	//verifica se é possivelcolocar peca
				ReleaseMutex(dados->hMutexTabuleiro);
				SetEvent(dados->hEventUpdateTabuleiro);

				if (colocou == TRUE) {
					_stprintf_s(a, MAX, _T("PECA %d %d %d\n"), x, y, t);
					escreveNamedPipe(dados, a, sourceTabuleiro);
					Sleep(200);
					if (_tcscmp(arrayComandos[0], LCLICK) == 0) {
						alteraSequencia(dados, sourceTabuleiro);
					}
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
		}
		else if (_tcscmp(arrayComandos[0], HOVER) == 0) {
			if (sourceTabuleiro->numParagensDisponiveis > 0) {
				sourceTabuleiro->numParagensDisponiveis--;
				SuspendThread(sourceTabuleiro->hThreadAgua);
				_stprintf_s(a, MAX, _T("HOVER\n"));
				escreveNamedPipe(dados, a, sourceTabuleiro);
				Sleep(200);
				_stprintf_s(a, MAX, _T("INFO AguaParada\n"));
				escreveNamedPipe(dados, a, sourceTabuleiro);
			}
		}
		else if (_tcscmp(arrayComandos[0], RETOMAHOVER) == 0) {
			_tprintf(_T("\n\n\RETOMA HOVER!!!\n\n"));
			ResumeThread(sourceTabuleiro->hThreadAgua);
			_stprintf_s(a, MAX, _T("FIM_HOVER\n"));
			escreveNamedPipe(dados, a, sourceTabuleiro);
			Sleep(200);
			_stprintf_s(a, MAX, _T("INFO RetomaAgua\n"));
			escreveNamedPipe(dados, a, sourceTabuleiro);
		}
		else if (_tcscmp(arrayComandos[0], JOGOSINGLEP) == 0) {
			if (dados->iniciado == FALSE) {//if jogo ainda não se encontra em curso
				_stprintf_s(sourceTabuleiro->pontuacao.nome, MAX, arrayComandos[1]);
				dados->velocidadeAgua = TIMER_FLUIR;
				sourceTabuleiro->pontuacao.vitorias = 0;
	
				_tprintf(_T("PREPARAR INICIO JOGO \n\n"));
				prepararInicioDeJogo(dados);
				sourceTabuleiro->correrAgua = TRUE;
				DadosThreadAgua dadosThreadAgua;
				dadosThreadAgua.dados = dados;
				dados->iniciado = TRUE;
				dadosThreadAgua.dadosTabuleiro = sourceTabuleiro;
				sourceTabuleiro->hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosThreadAgua, 0, NULL);
				if (sourceTabuleiro->hThreadAgua == NULL) {
					_tprintf(_T("NAO CONSEGUI CRIAR NOVA THREAD\n\n"));
				}

				WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
				_stprintf_s(a, MAX, _T("INICIAJOGO %d %d %d %d %d %d %d %d\n"), dados->memPar->tamX, dados->memPar->tamY, sourceTabuleiro->posX, sourceTabuleiro->posY, (*sourceTabuleiro->tabuleiro)[sourceTabuleiro->posX][sourceTabuleiro->posY], dados->posfX, dados->posfY, (*sourceTabuleiro->tabuleiro)[dados->posfX][dados->posfY]);
				ReleaseMutex(dados->hMutexTabuleiro);
				escreveNamedPipe(dados, a, sourceTabuleiro);
			}
			else {
				escreveNamedPipe(dados, _T("INFO JogoEmCurso\n"), sourceTabuleiro);
			}
		}
		else if (_tcscmp(arrayComandos[0], JOGOMULTIP) == 0) {
			if (dados->iniciado == FALSE) {//if jogo ainda não se encontra em curso
				_stprintf_s(sourceTabuleiro->pontuacao.nome, MAX, arrayComandos[1]);
				sourceTabuleiro->pontuacao.vitorias = 0;
				if (multi > 0){			//existe um jogador em espera
					dados->velocidadeAgua = TIMER_FLUIR;
					dados->iniciado = TRUE;
					dados->tabuleiro1.correrAgua = TRUE;
					dados->tabuleiro2.correrAgua = TRUE;

					DadosThreadAgua dadosThreadAguaTab1;
					dadosThreadAguaTab1.dados = dados;
					dadosThreadAguaTab1.dadosTabuleiro = &dados->tabuleiro1;
					dados->tabuleiro1.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosThreadAguaTab1, CREATE_SUSPENDED, NULL);
					if (dados->tabuleiro1.hThreadAgua == NULL) {
						_tprintf(_T("NAO CONSEGUI CRIAR NOVA THREAD\n\n"));
						exit(-1);
					}
					DadosThreadAgua dadosThreadAguaTab2;
					dadosThreadAguaTab2.dados = dados;
					dadosThreadAguaTab2.dadosTabuleiro = &dados->tabuleiro2;
					dados->tabuleiro2.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosThreadAguaTab2, CREATE_SUSPENDED, NULL);
					//iniciajogo peçainicialx pecainicialy tipopeçainicial peçafinalx pecafinaly tipopeçafinal
					if (sourceTabuleiro->hThreadAgua == NULL) {
						_tprintf(_T("NAO CONSEGUI CRIAR NOVA THREAD\n\n"));
						exit(-1);
					}
					ResumeThread(dados->tabuleiro1.hThreadAgua);
					ResumeThread(dados->tabuleiro2.hThreadAgua);

					WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
					_stprintf_s(a, MAX, _T("INICIAJOGO %d %d %d %d %d %d %d %d\n"), dados->memPar->tamX, dados->memPar->tamY, sourceTabuleiro->posX, sourceTabuleiro->posY, (*sourceTabuleiro->tabuleiro)[sourceTabuleiro->posX][sourceTabuleiro->posY], dados->posfX, dados->posfY, (*sourceTabuleiro->tabuleiro)[dados->posfX][dados->posfY]);
					ReleaseMutex(dados->hMutexTabuleiro);
					escreveNamedPipe(dados, a, &dados->tabuleiro1);
					Sleep(100);
					escreveNamedPipe(dados, a, &dados->tabuleiro2);
				}
				else {
					multi++;
				}
			}
			else {
				escreveNamedPipe(dados, _T("INFO JogoEmCurso\n"), sourceTabuleiro);
			}
		}
		else if (_tcscmp(arrayComandos[0], JOGOMULTIPCANCEL) == 0) {
			multi--;
		}
		else if (_tcscmp(arrayComandos[0], PROXNIVEL) == 0) {
			if (dados->iniciado == TRUE) {//if jogo se encontra em curso
				if (!sourceTabuleiro->correrAgua) {
					_tprintf(_T("PREPARAR INICIO JOGO \n\n"));
					prepararInicioDeJogo(dados);
					_tprintf(_T("ESPERAR PELA MORTE DA THREAD\n\n"));
					if (WaitForSingleObject(sourceTabuleiro->hThreadAgua, 2000) == WAIT_TIMEOUT) {
						TerminateThread(sourceTabuleiro->hThreadAgua, -1);
						_tprintf(_T("TIVE DE MATAR A FORÇA\n\n"));
					}
					sourceTabuleiro->correrAgua = TRUE;
					DadosThreadAgua dadosThreadAgua;
					dadosThreadAgua.dados = dados;
					dados->iniciado = TRUE;
					dadosThreadAgua.dadosTabuleiro = sourceTabuleiro;
					sourceTabuleiro->hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosThreadAgua, 0, NULL);
					//iniciajogo peçainicialx pecainicialy tipopeçainicial peçafinalx pecafinaly tipopeçafinal
					if (sourceTabuleiro->hThreadAgua == NULL) {
						_tprintf(_T("NAO CONSEGUI CRIAR NOVA THREAD\n\n"));
					}
					
					WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
					_stprintf_s(a, MAX, _T("INICIAJOGO %d %d %d %d %d %d %d %d\n"), dados->memPar->tamX, dados->memPar->tamY, sourceTabuleiro->posX, sourceTabuleiro->posY, (*sourceTabuleiro->tabuleiro)[sourceTabuleiro->posX][sourceTabuleiro->posY], dados->posfX, dados->posfY, (*sourceTabuleiro->tabuleiro)[dados->posfX][dados->posfY]);
					ReleaseMutex(dados->hMutexTabuleiro);
					escreveNamedPipe(dados, a, sourceTabuleiro);
				}
				else {
					escreveNamedPipe(dados, _T("INFO JogoEstaACorrer\n"), sourceTabuleiro);
				}
			}
			else {
				escreveNamedPipe(dados, _T("INFO JogoNaoEstaIniciado\n"), sourceTabuleiro);
			}
		}
		else if (_tcscmp(arrayComandos[0], SAIRCLI) == 0) {
			DisconnectNamedPipe(sourceTabuleiro->pipes.hPipeIn);
			DisconnectNamedPipe(sourceTabuleiro->pipes.hPipeOut);

			sourceTabuleiro->pipes.hPipeOut = CreateNamedPipe(NOME_PIPE_SERVIDOR, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
			if (sourceTabuleiro->pipes.hPipeOut == INVALID_HANDLE_VALUE) {
				_tprintf(TEXT("[ERRO] Criar Named Pipe S->C! (CreateNamedPipe)"));
			}

			sourceTabuleiro->pipes.hPipeIn = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, MAX * sizeof(TCHAR), MAX * sizeof(TCHAR), 1000, NULL);
			if (sourceTabuleiro->pipes.hPipeIn == NULL) {
				_tprintf(TEXT("[ERRO] Criar instacia do pipe '%s'! (CreateNamedPipe)\n"), NOME_PIPE_CLIENTE);
			}

			dados->iniciado = FALSE;

			//se for multiplayer -> informar ao outro cliente que ganhou
		}
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

BOOL verificaColocarPeca(DadosThread* dados, int x, int y, int t, DadosTabuleiro* sourceTabuleiro) {		//adicionar depois qual o tabuleiro a verificar (int tab)
	//verificar se celula é água ou verificar se célula é o fim jogo ou barr
	if (x >= dados->memPar->tamX || y >= dados->memPar->tamY)
	{
		return FALSE;
	}
	
	if (x == dados->posfX && y == dados->posfY )
		return FALSE;

	WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
	BOOL ret;
	if ((* sourceTabuleiro->tabuleiro)[x][y] < 0 || (*sourceTabuleiro->tabuleiro)[x][y] == 7) {
		ret = FALSE;
	}
	else {
		(*sourceTabuleiro->tabuleiro)[x][y] = t;
		ret = TRUE;
	}
	ReleaseMutex(dados->hMutexTabuleiro);

	return ret;
}


/*****************************************/
/*************ThreadEscrever**************/
/*****************************************/
DWORD WINAPI ThreadEscrever(LPVOID param) {								//thread escritura de informações para o cliente através do named pipe
	DadosThread* dados = (DadosThread*)param;
	DWORD n;
	TCHAR buf[MAX]=_T("yodvbasucansdo");
	int i;

	do {
		//fica bloqueado à espera de um evento 
		WaitForSingleObject(dados->hEventoNamedPipe, INFINITE);

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


//+++++++++++++++++++++++++++++++++++++++
//++++++++++ThreadConsumidor+++++++++++++
//+++++++++++++++++++++++++++++++++++++++
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
				TCHAR a[MAX];
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
								
								_stprintf_s(a, MAX, _T("PECA %d %d 7\n"), x,y);
								escreveNamedPipe(dados, a, &dados->tabuleiro1);
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
								_stprintf_s(a, MAX, _T("PECA %d %d 7\n"), x, y);
								escreveNamedPipe(dados, a, &dados->tabuleiro2);
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
				dados->modoRandom = !dados->modoRandom;
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
		(*dados->tabuleiro1.tabuleiro)[posInicio][0] = tipoTubo * -1;						//mete tubo correto na posição de inicio
		(*dados->tabuleiro2.tabuleiro)[posInicio][0] = tipoTubo * -1;						//mete tubo com água correto na posição de inicio

		dados->tabuleiro1.posX = posInicio;													//posição de agua ativa = posição de inicio
		dados->tabuleiro1.posY = 0;
		dados->tabuleiro2.posX = posInicio;													//posição de agua ativa = posição de inicio
		dados->tabuleiro2.posY = 0;
		
		(*dados->tabuleiro1.tabuleiro)[posFim][dados->memPar->tamY - 1] = tipoTubo;			//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		(*dados->tabuleiro2.tabuleiro)[posFim][dados->memPar->tamY - 1] = tipoTubo;			//posFim -> Parede contrária ao inicio			//devemos querer guardar esta posição também nos dados do servidor
		
		dados->tabuleiro1.dirAgua = BAIXO;
		dados->tabuleiro2.dirAgua = BAIXO;

		dados->posfX = posFim;
		dados->posfY = dados->memPar->tamY - 1;

		break;
	case 1:
		ultimaPos = dados->memPar->tamX - 1;											//pareda lado direito
		(*dados->tabuleiro1.tabuleiro)[ultimaPos][posInicio] = tipoTubo * -1;
		(*dados->tabuleiro2.tabuleiro)[ultimaPos][posInicio] = tipoTubo * -1;

		dados->tabuleiro1.posX = ultimaPos;
		dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = ultimaPos;
		dados->tabuleiro2.posY = posInicio;
		
		(*dados->tabuleiro1.tabuleiro)[0][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[0][posFim] = tipoTubo;

		dados->tabuleiro1.dirAgua = ESQUERDA;
		dados->tabuleiro2.dirAgua = ESQUERDA;

		dados->posfX = 0;
		dados->posfY = posFim;

		break;
	case 2:
		ultimaPos = dados->memPar->tamY - 1;											//parede baixo
		(*dados->tabuleiro1.tabuleiro)[posInicio][ultimaPos] = tipoTubo * -1;
		(*dados->tabuleiro2.tabuleiro)[posInicio][ultimaPos] = tipoTubo * -1;

		dados->tabuleiro1.posX = posInicio;
		dados->tabuleiro1.posY = ultimaPos;
		dados->tabuleiro2.posX = posInicio;
		dados->tabuleiro2.posY = ultimaPos;

		(*dados->tabuleiro1.tabuleiro)[posFim][0] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[posFim][0] = tipoTubo;

		dados->tabuleiro1.dirAgua = CIMA;
		dados->tabuleiro2.dirAgua = CIMA;

		dados->posfX = posFim;
		dados->posfY = 0;
		
		break;
	case 3:
		(*dados->tabuleiro1.tabuleiro)[0][posInicio] = tipoTubo * -1;					//parede lado esquerdo
		(*dados->tabuleiro2.tabuleiro)[0][posInicio] = tipoTubo * -1;

		dados->tabuleiro1.posX = 0;
		dados->tabuleiro1.posY = posInicio;
		dados->tabuleiro2.posX = 0;
		dados->tabuleiro2.posY = posInicio;

		(*dados->tabuleiro1.tabuleiro)[dados->memPar->tamX - 1][posFim] = tipoTubo;
		(*dados->tabuleiro2.tabuleiro)[dados->memPar->tamX - 1][posFim] = tipoTubo;

		dados->tabuleiro1.dirAgua = DIREITA;
		dados->tabuleiro2.dirAgua = DIREITA;

		dados->posfX = dados->memPar->tamX - 1;
		dados->posfY = posFim;
		
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
	DadosTabuleiro* dadosTabuleiro = paramStruct->dadosTabuleiro;

	SetEvent(dados->hEventUpdateTabuleiro);														//assinala evento de atualização do tabuleiro
	_tprintf(_T("(ThreadAgua) Sleep %d"), dados->tempoInicioAgua * 1000);			
	Sleep(dados->tempoInicioAgua * 1000);														//jogo incia após "tempoInicioAgua"

		while (dadosTabuleiro->correrAgua) {
			if (dados->parafluxo == 0) {
				WaitForSingleObject(dados->hMutexTabuleiro, INFINITE);
				if (fluirAgua(dados, dadosTabuleiro)) {															//verifica se a água fluiu com sucesso
					if (dadosTabuleiro->posX == dados->posfX && dadosTabuleiro->posY == dados->posfY){
						_tcscpy_s(dados->memPar->estado, MAX, _T("Ganhou!!!"));					//muda o estado do jogo 
						_tprintf(_T("(ThreadAgua) Ganhou!!!"));
						dadosTabuleiro->pontuacao.vitorias++;
						dados->velocidadeAgua *= AUMENTO_VELOCIDADE;
						escreveNamedPipe(dados, _T("GANHOU 10\n"), dadosTabuleiro);
						Sleep(200);
						TCHAR a[MAX];
						_stprintf_s(a, MAX, _T("PECA %d %d %d\n"), dadosTabuleiro->posX, dadosTabuleiro->posY, (*dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY]);
						escreveNamedPipe(dados, a, dadosTabuleiro);
						SetEvent(dados->hEventUpdateTabuleiro);
						ReleaseMutex(dados->hMutexTabuleiro);
						dadosTabuleiro->correrAgua = FALSE;
						//dados->iniciado = FALSE;
						break;
					}
					else {
						_tcscpy_s(dados->memPar->estado, MAX, _T("Água fluiu"));				//muda o estado do jogo 
						TCHAR a[MAX];
						_stprintf_s(a, MAX, _T("PECA %d %d %d\n"), dadosTabuleiro->posX, dadosTabuleiro->posY, (* dadosTabuleiro->tabuleiro)[dadosTabuleiro->posX][dadosTabuleiro->posY]);
						escreveNamedPipe(dados, a, dadosTabuleiro);
						/*_tprintf(_T("(ThreadAgua) Fluir água! Sleep %d"), TIMER_FLUIR * 1000);
						escreveNamedPipe(dados, _T("INFO FluiAgua\n"), dadosTabuleiro);*/
					}
					ReleaseMutex(dados->hMutexTabuleiro);
					SetEvent(dados->hEventUpdateTabuleiro);
					Sleep(dados->velocidadeAgua * 1000);
				}
				else {
					_tcscpy_s(dados->memPar->estado, MAX, _T("Perdeu!!!"));						//muda o estado do jogo 
					_tprintf(_T("(ThreadAgua) Perdeu!!!"));
					dados->velocidadeAgua = TIMER_FLUIR;
					escreveNamedPipe(dados, _T("PERDEU 10\n"), dadosTabuleiro);
					ReleaseMutex(dados->hMutexTabuleiro);
					SetEvent(dados->hEventUpdateTabuleiro);
					_tprintf(_T("(ThreadAgua) FluirAgua -> FALSE"));
					dadosTabuleiro->correrAgua = FALSE;
					dados->iniciado = FALSE;
					break;
				}
			}
			else {
				Sleep(dados->parafluxo * 1000);													//tempo do fluxo da água
				dados->parafluxo = 0;
			}
		}
	//dados->iniciado = FALSE;
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

	dados->memPar->tamX = tamH;
	dados->memPar->tamY = tamV;
		
	//dados->memPar->tamX = 10;
	//dados->memPar->tamY = 7;

	dados->tabuleiro1.tabuleiro = &(dados->memPar->dadosTabuleiro1.tabuleiro);
	dados->tabuleiro1.jogadorAtivo = &(dados->memPar->dadosTabuleiro1.jogadorAtivo);
	dados->tabuleiro2.tabuleiro = &(dados->memPar->dadosTabuleiro2.tabuleiro);
	dados->tabuleiro2.jogadorAtivo = &(dados->memPar->dadosTabuleiro2.jogadorAtivo);
	/*
	for (int i = 0; i < tamH; i++) {
		for (int j = 0; j < tamV; j++) {
			(*dados->tabuleiro1.tabuleiro)[i][j] = 0;

			(*dados->tabuleiro2.tabuleiro)[i][j] = 0;		//apenas meta 1. Depois será feito -> tabuleiro2 = tabuleiro1 (cópia de valores)
		}
	}
	*/
	*dados->tabuleiro1.jogadorAtivo = FALSE;
	*dados->tabuleiro2.jogadorAtivo = FALSE;


	//	MAPA PRÉ-DEFINIDO
	//carregaMapaPreDefinido(dados, dados->tabuleiro1.tabuleiro);
	
	//*dados->tabuleiro1.jogadorAtivo = TRUE;

	// Define início e fim
	prepararInicioDeJogo(dados);
	//definirInicioFim(dados);


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
	dados->tabuleiro1.pipes.hPipeOut = CreateNamedPipe(NOME_PIPE_SERVIDOR, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeOut == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe S->C! (CreateNamedPipe)"));
		ReleaseMutex(dados->hMutexNamedPipe);
		CloseHandle(dados->hMutexNamedPipe);
		return FALSE;
	}
	
	dados->tabuleiro2.pipes.hPipeOut = CreateNamedPipe(NOME_PIPE_SERVIDOR, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeOut == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe S->C! (CreateNamedPipe)"));
		ReleaseMutex(dados->hMutexNamedPipe);
		CloseHandle(dados->hMutexNamedPipe);
		return FALSE;
	}
	ReleaseMutex(dados->hMutexNamedPipe);

	dados->tabuleiro1.pipes.hPipeIn = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, MAX * sizeof(TCHAR), MAX * sizeof(TCHAR), 1000, NULL);
	if (dados->tabuleiro1.pipes.hPipeIn == NULL) {
		_tprintf(TEXT("[ERRO] Criar instacia do pipe '%s'! (CreateNamedPipe)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}

	dados->tabuleiro2.pipes.hPipeIn = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 20, MAX * sizeof(TCHAR), MAX * sizeof(TCHAR), 1000, NULL);
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
	
	//DadosThreadAgua dadosAguaTabuleiro1;
	//dadosAguaTabuleiro1.dados = &dados;
	//dadosAguaTabuleiro1.dadosTabuleiro = &dados.tabuleiro1;
	//dados.tabuleiro1.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosAguaTabuleiro1, CREATE_SUSPENDED, NULL);					//thread criada suspensa até que monitor inicie o jogo


	//DadosThreadAgua dadosAguaTabuleiro2;
	//dadosAguaTabuleiro2.dados = &dados;
	//dadosAguaTabuleiro2.dadosTabuleiro = &dados.tabuleiro2;
	//dados.tabuleiro2.hThreadAgua = CreateThread(NULL, 0, ThreadAgua, &dadosAguaTabuleiro2, CREATE_SUSPENDED, NULL);					//thread criada suspensa até que monitor inicie o jogo


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
		TCHAR a[MAX];

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

					_stprintf_s(a, MAX, _T("SUSPENDE 1\n"));
					if (dados.tabuleiro1.jogadorAtivo)
						escreveNamedPipe(&dados, a, &dados.tabuleiro1);
					if (dados.tabuleiro2.jogadorAtivo)
						escreveNamedPipe(&dados, a, &dados.tabuleiro2);
				}
				else {
					_tprintf(TEXT("Jogo não está em curso\n"));
				}
			}
			else if (_tcscmp(comando, RETOMAR) == 0) {
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

					_stprintf_s(a, MAX, _T("RETOMA 1\n"));
					if (dados.tabuleiro1.jogadorAtivo)
						escreveNamedPipe(&dados, a, &dados.tabuleiro1);
					if (dados.tabuleiro2.jogadorAtivo)
						escreveNamedPipe(&dados, a, &dados.tabuleiro2);
				}
				else {
					_tprintf(TEXT("Jogo não está em curso\n"));
				}
			}
			else if (_tcscmp(comando, PONTUACAO) == 0) {
				if (!*(dados.tabuleiro1.jogadorAtivo) && !*(dados.tabuleiro1.jogadorAtivo)) {
					_tprintf(TEXT("Nenhum jogador ativo\n"), dados.tabuleiro1.pontuacao.nome, dados.tabuleiro1.pontuacao.vitorias);
				}


				if (*(dados.tabuleiro1.jogadorAtivo)){
					_tprintf(TEXT("Jogador 1: %s\nVitórias: %d\n"), dados.tabuleiro1.pontuacao.nome, dados.tabuleiro1.pontuacao.vitorias);
				}
				if (*dados.tabuleiro2.jogadorAtivo) {
					_tprintf(TEXT("Jogador 2: %s\nVitórias: %d\n"), dados.tabuleiro2.pontuacao.nome, dados.tabuleiro2.pontuacao.vitorias);
				}
			}
		} while (_tcscmp(comando, SAIR) != 0);

		_stprintf_s(a, MAX, _T("SAIR 1\n"));
		if (dados.tabuleiro1.jogadorAtivo)
			escreveNamedPipe(&dados, a, &dados.tabuleiro1);
		if (dados.tabuleiro2.jogadorAtivo)
			escreveNamedPipe(&dados, a, &dados.tabuleiro2);

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