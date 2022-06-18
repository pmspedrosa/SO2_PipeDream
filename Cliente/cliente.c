#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "cliente.h"
#define TAM_BITMAP 150
#define NUM_BITMAPS 14


LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base2022");

TCHAR** divideString(TCHAR* comando, const TCHAR* delim, unsigned int* tam) {
	TCHAR* proxToken = NULL, ** temp, ** arrayCmd = NULL;
	TCHAR* token = _tcstok_s(comando, delim, &proxToken);

	if (comando == NULL || _tcslen(comando) == 0) {						//verifica se string está vazia
		_ftprintf(stderr, TEXT("[ERRO] String comando vazia!"));
		return NULL;
	}

	*tam = 0;

	while (token != NULL) {
		//realocar a memória para integrar mais um argumento
		//realloc retorna um ponteiro para a nova memoria alocada, ou NULL quando falha
		temp = realloc(arrayCmd, sizeof(TCHAR*) * (*tam + 1));

		if (temp == NULL) {
			_ftprintf(stderr, TEXT("[ERRO] Falha ao alocar memoria!"));
			return NULL;
		}
		arrayCmd = temp;												//apontar para a nova memoria alocada				
		arrayCmd[(*tam)++] = token;

		token = _tcstok_s(NULL, delim, &proxToken);						//copia o proximo token para a "token"
	}
	return arrayCmd;
}



DWORD WINAPI ThreadLer(LPVOID param) {						
	//fica sempre á escuta de ler coisas vindas do named pipe
	//escreve diretamente no ecrã
	DadosThreadPipe* dados = (DadosThreadPipe*)param;
	TCHAR buf[MAX], ** arrayComandos = NULL;
	DWORD n;
	unsigned int nrArgs = 0;
	const TCHAR delim[2] = _T(" ");

	OutputDebugString(TEXT("[Cliente] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);

	//espera que exista um named pipe para ler do mesmo
	//bloqueia aqui
	if (!WaitNamedPipe(NOME_PIPE_SERVIDOR, NMPWAIT_WAIT_FOREVER)) {
		OutputDebugString(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	OutputDebugString(TEXT("[Cliente] Ligação ao pipe do servidor... (CreateFile)\n"));

	HANDLE hPipe = CreateFile(NOME_PIPE_SERVIDOR, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hPipe == NULL) {
		OutputDebugString(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	OutputDebugString(TEXT("[Cliente] Liguei-me...\n"));

	while (dados->terminar == 0) {
		//le as mensagens enviadas pelo cliente
		BOOL ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
		buf[n / sizeof(TCHAR)] = '\0';

		if (!ret || !n) {
			OutputDebugString(TEXT("[Cliente] %d %d... (ReadFile)\n"), ret, n);
			break;
		}

		OutputDebugString(TEXT("[Cliente] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
		
		// comando arg0 arg1 ....
		//msg= info_0_barreira colocada local tal e tal
		
		arrayComandos = divideString(buf, delim, &nrArgs);			//divisão da string para um array com o comando e args
		/*
		#define INFO _T("INFO")
		#define PEÇA _T("PEÇA")
		#define SEQ _T("SEQ")
		#define SUSPENDE _T("SUSPENDE")
		#define RETOMA _T("RETOMA")
		#define SAIR _T("SAIR")
		*/
		if (_tcscmp(arrayComandos[0], INFO) == 0) {
			//to do
		}
		else if (_tcscmp(arrayComandos[0], PECA) == 0) {
			if (nrArgs >= 3) {	//x,y,tipopeça
				unsigned int x, y, t;
				if (_tcscmp(arrayComandos[1], _T("0")) != 0) {		//verifica se valor não é igual a '0' pois atoi devolve 0 quando é erro
					x = _tstoi(arrayComandos[1]);
					if (x == 0) {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
				}
				else
					x = 0;
				if (_tcscmp(arrayComandos[2], _T("0")) != 0) {	
					y = _tstoi(arrayComandos[2]);
					if (y == 0) {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
				}
				else
					y = 0;
				if (_tcscmp(arrayComandos[3], _T("0")) != 0) {	
					t = _tstoi(arrayComandos[3]);
					if (t == 0) {
						_tprintf(_T("Valor passado como argumento não é aceite\n"));
					}
				}
				else
					t = 0;
				WaitForSingleObject(dados->hMutex, INFINITE);
				dados->tabuleiro[x][y] = t;
				ReleaseMutex(dados->hMutex);
				InvalidateRect(dados->hWnd, NULL, TRUE);        //Chama WM_PAINT
			}
		}
		else if (_tcscmp(arrayComandos[0], SEQ) == 0) {			//msg: seq tipopeça1 tipopeça2 ...
			if (nrArgs >= 6) {
				for (int i = 1; i < 7; i++)
				{
					int t = 0;
					if (_tcscmp(arrayComandos[3], _T("0")) != 0) {
						t = _tstoi(arrayComandos[3]);
						if (t == 0) {
							_tprintf(_T("Valor passado como argumento não é aceite\n"));
						}
					}
					WaitForSingleObject(dados->hMutex, INFINITE);
					dados->seq[i-1] = t;
					ReleaseMutex(dados->hMutex);
					InvalidateRect(dados->hWnd, NULL, TRUE);        //Chama WM_PAINT
				}	
			}
		}
		else if (_tcscmp(arrayComandos[0], SUSPENDE) == 0) {
			//cria uma textbox com a informação de que o jogo está parado

		}
		else if (_tcscmp(arrayComandos[0], RETOMA) == 0) {
			//desativa a textbox e continua o jogo

		}
		else if (_tcscmp(arrayComandos[0], SAIR) == 0) {
			//fechar tudoooo
		}

	}

	CloseHandle(hPipe);

	return 0;
}


DWORD WINAPI ThreadEscrever(LPVOID param) {								//thread escritura de informações para o servidor através do named pipe
	DadosThreadPipe* dados = (DadosThreadPipe*)param;
	DWORD n;

	do {

		//fica bloqueado à espera do evento
		WaitForSingleObject(dados->hEventoNamedPipe, INFINITE);

		WaitForSingleObject(dados->hMutex, INFINITE);
		if (dados->hPipe.activo) {
			if (!WriteFile(dados->hPipe.hPipe, dados->mensagem, sizeof(dados->mensagem), &n, NULL))
				OutputDebugString(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			else
				OutputDebugString(TEXT("[ESCRITOR] Enviei %d bytes ao cliente ... (WriteFile)\n"), n);
		}
		ReleaseMutex(dados->hMutex);
		ResetEvent(dados->hEventoNamedPipe);

	} while (dados->terminar == 0);
	return 0;
}


BOOL initNamedPipes(DadosThreadPipe* dados) {
	HANDLE hPipe, hThread;
	dados->terminar = 0;

	dados->hMutex = CreateMutex(NULL, FALSE, MUTEX_NPIPE_CLI); //Criação do mutex
	if (dados->hMutex == NULL) {
		OutputDebugString(TEXT("[Erro] ao criar mutex!\n"));
		return FALSE;
	}

	//hPipe = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 3, MAX * sizeof(TCHAR),MAX * sizeof(TCHAR), 1000, NULL);
	
	hPipe = CreateFile(NOME_PIPE_CLIENTE, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		OutputDebugString(TEXT("[ERRO] Ligar ao Pipe! (CreateFile)"));
		CloseHandle(dados->hMutex);
		return FALSE;
	}

	WaitForSingleObject(dados->hMutex, INFINITE);
	dados->hPipe.hPipe = hPipe;
	dados->hPipe.activo = FALSE;
	ReleaseMutex(dados->hMutex);

	_tprintf(TEXT("[CLIENTE] Esperar ligação de um servidor... (ConnectNamedPipe)\n"));
	//o cliente espera até ter um servidor conectado a esta instância
	/*
	if (!ConnectNamedPipe(dados->hPipe.hPipe, NULL)) {
		OutputDebugString(TEXT("[ERRO] Ligação ao servidor! (ConnectNamedPipe\n"));
		CloseHandle(dados->hMutex);
		DisconnectNamedPipe(dados->hPipe.hPipe);
		return FALSE;
	}
	*/
	//_tprintf(TEXT(" Ligação ao servidor com sucesso! (ConnectNamedPipe\n"));
	OutputDebugString(TEXT(" Ligação ao servidor com sucesso! (ConnectNamedPipe\n"));
	WaitForSingleObject(dados->hMutex, INFINITE);
	dados->hPipe.activo = TRUE;
	ReleaseMutex(dados->hMutex);

	dados->hEventoNamedPipe = CreateEvent(NULL, TRUE, FALSE, EVENT_NAMEDPIPE_SV);
	if (dados->hEventoNamedPipe == NULL) {
		OutputDebugString(_T("Erro: CreateEvent NamedPipe(%d)\n"), GetLastError());
		CloseHandle(dados->hMutex);
		DisconnectNamedPipe(dados->hPipe.hPipe);
		return FALSE;
	}
}




int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;		// hWnd � o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg;		// MSG � uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;	// WNDCLASSEX � uma estrutura cujos membros servem para 
			  // definir as caracter�sticas da classe da janela
	
	//verificar se existe sv ativo
	HANDLE hEventUpdateTabuleiro = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TABULEIRO);

	if (hEventUpdateTabuleiro == NULL) {
		OutputDebugString(_T("Erro: OpenEvent (%d)\n"), GetLastError());
		//dados->terminar = 1;
		return 0;
	}

	wcApp.cbSize = sizeof(WNDCLASSEX);      // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;		         // Inst�ncia da janela actualmente exibida 
								   // ("hInst" � par�metro de WinMain e vem 
										 // inicializada da�)
	wcApp.lpszClassName = szProgName;       // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos;       // Endere�o da fun��o de processamento da janela
											// ("TrataEventos" foi declarada no in�cio e
											// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for
											// modificada horizontal ou verticalmente

	wcApp.hIcon = LoadIcon(NULL, IDI_QUESTION);   // "hIcon" = handler do �con normal
										   // "NULL" = Icon definido no Windows
										   // "IDI_AP..." �cone "aplica��o"
	wcApp.hIconSm = LoadIcon(NULL, IDI_SHIELD); // "hIconSm" = handler do �con pequeno
										   // "NULL" = Icon definido no Windows
										   // "IDI_INF..." �con de informa��o
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
							  // "NULL" = Forma definida no Windows
							  // "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = NULL;			// Classe do menu que a janela pode ter
							  // (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0;				// Livre, para uso particular
	wcApp.cbWndExtra = 0;				// Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	if (!RegisterClassEx(&wcApp))
		return(0);

	hWnd = CreateWindow(
		szProgName,			// Nome da janela (programa) definido acima
		TEXT("Exemplo de Janela Principal em C"),// Texto que figura na barra do t�tulo
		WS_OVERLAPPEDWINDOW,	// Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT,		// Posi��o x pixels (default=� direita da �ltima)
		CW_USEDEFAULT,		// Posi��o y pixels (default=abaixo da �ltima)
		CW_USEDEFAULT,		// Largura da janela (em pixels)
		CW_USEDEFAULT,		// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de
						// outra) ou HWND_DESKTOP se a janela for a primeira, 
						// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da inst�ncia do programa actual ("hInst" � 
						// passado num dos par�metros de WinMain()
		0);				// N�o h� par�metros adicionais para a janela
	  // ============================================================================
	  // 4. Mostrar a janela
	  // ============================================================================
	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por 
					  // "CreateWindow"; "nCmdShow"= modo de exibi��o (p.e. 
					  // normal/modal); � passado como par�metro de WinMain()
	UpdateWindow(hWnd);		// Refrescar a janela (Windows envia � janela uma 
					  // mensagem para pintar, mostrar dados, (refrescar)� 

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);	// Pr�-processamento da mensagem (p.e. obter c�digo 
					   // ASCII da tecla premida)
		DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
					   // aguarda at� que a possa reenviar � fun��o de 
					   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}
	return((int)lpMsg.wParam);	// Retorna sempre o par�metro wParam da estrutura lpMsg
}

typedef struct PosChar {
	DWORD xPos, yPos;
	TCHAR c;
}PosChar;

typedef struct BitmapInfo {
	BITMAP bmp;
	HDC bmpDC;
}BitmapInfo;

void loadImages(BOOL set, BitmapInfo bitmap[], HWND hWnd) {
	HBITMAP hBmp;
	HDC hdc = GetDC(hWnd);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe6_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[0].bmp);
	bitmap[0].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[0].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe5_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[1].bmp);
	bitmap[1].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[1].bmpDC, hBmp);
	
	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe4_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[2].bmp);
	bitmap[2].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[2].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe3_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[3].bmp);
	bitmap[3].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[3].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe2_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[4].bmp);
	bitmap[4].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[4].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe1_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[5].bmp);
	bitmap[5].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[5].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe0.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[6].bmp);
	bitmap[6].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[6].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe1.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[7].bmp);
	bitmap[7].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[7].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe2.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[8].bmp);
	bitmap[8].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[8].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe3.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[9].bmp);
	bitmap[9].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[9].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe4.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[10].bmp);
	bitmap[10].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[10].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe5.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[11].bmp);
	bitmap[11].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[11].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe6.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[12].bmp);
	bitmap[12].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[12].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\pipe0.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[13].bmp);
	bitmap[13].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[13].bmpDC, hBmp);
}

int getPaddings(int tamX, int tamY, RECT* rect, int* paddingX, int* paddingY, int* larguraSeq, int* paddingSeq) {			
	// Recebe dimensoes do tabuleiro, o rect do ecrã e ponteiros para as variáveis que guardaram os paddings 
	// Recebe também ponteiro para variaveis que guardam o padding vertical da barra de sequencia e a largura da barra de sequencia (largura da barra indica o tamanho de celula da barra de sequencia)
	// Devolve o tamanho de célula

	*larguraSeq = rect->right / 5;								//largura máxima de barra de sequencia é 1/5 do ecrâ
	int tempPaddingSeq = (rect->bottom-ALTURA_INFO) / 6;						//altura maxima de cada celula (uma vez que há 6 celulas na sequencia)


	if (*larguraSeq >= tempPaddingSeq) {							// se tamanho maximo de celula em largura >= tamanho maximo de celula em altura 
		*larguraSeq = tempPaddingSeq;								// tamanho de celula = altura máxima
		tempPaddingSeq = 0;											// uma vez que o tamanho de celula é ditado pela altura, a barra vai ocupar a altura inteira do ecrã
	}
	else {															// se tamanho maximo de celula em largura < tamanho maximo de celula em altura
																	// tamanho de celula = largura maxima (logo não é preciso mudar o valor de *larguraSeq)
		tempPaddingSeq = (rect->bottom - (6 * (*larguraSeq))) / 2;	// padding é igual a metade da diferenca entre a altura disponivel e a altura necessaria para mostrar a barra de sequencia (6 celulas)
	}
	if (paddingSeq != NULL){
		*paddingSeq = tempPaddingSeq;
	}

	int tamCelula;
	*paddingX = (rect->right - *larguraSeq) / tamX;
	*paddingY = (rect->bottom - ALTURA_INFO) / tamY;

	if (*paddingX >= *paddingY) {
		tamCelula = *paddingY;
		*paddingX = ((rect->right - *larguraSeq) - (tamCelula * tamX)) / 2;
		*paddingY = 0;
	}
	else {
		tamCelula = *paddingX;
		*paddingY = (rect->bottom - tamCelula * tamY) / 2;
		*paddingX = 0;
	}

	return tamCelula;
}

void atualizarDisplay(HWND hWnd, DadosThreadPipe* dados, BitmapInfo bitmap[]) {
	RECT rect;
	HDC hdc = GetDC(hWnd);
	GetClientRect(hWnd, &rect);
	int paddingX, paddingY, paddingSeq, larguraSeq;
	int tamCelula = getPaddings(dados->tamX, dados->tamY, &rect, &paddingX, &paddingY, &larguraSeq, &paddingSeq);

	BITMAP currBitmap;


	FillRect(hdc, &rect, CreateSolidBrush(RGB(100, 100, 100)));
	
	//BARRA DE SEQUENCIA
	for (int i = 0; i < 6; i++) {
		currBitmap = bitmap[dados->seq[i] + 6].bmp;
		StretchBlt(hdc, 0, (larguraSeq * i) + paddingSeq + ALTURA_INFO, larguraSeq, larguraSeq, bitmap[dados->seq[i] + 6].bmpDC, 0, 0, currBitmap.bmWidth, currBitmap.bmHeight, SRCCOPY);
	}
	
	//TABULEIRO
	for (int x = 0; x < dados->tamX; x++){
		for (int y = 0; y < dados->tamY; y++){
			currBitmap = bitmap[dados->tabuleiro[x][y] + 6].bmp;
			StretchBlt(hdc, (tamCelula * x) + paddingX + larguraSeq, (tamCelula * y) + paddingY + ALTURA_INFO, tamCelula, tamCelula, bitmap[dados->tabuleiro[x][y] + 6].bmpDC, 0, 0, currBitmap.bmWidth, currBitmap.bmHeight, SRCCOPY);
		}
	}

	//PRINT INFO
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, TRANSPARENT);
	rect.left = 0;
	rect.top = 0;
	DrawText(hdc, &dados->mensagem, -1, &rect, DT_SINGLELINE, DT_NOCLIP);
}


BOOL iniciaDetecaoHover(HWND hWnd) {
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_HOVER;
	tme.hwndTrack = hWnd;
	tme.dwHoverTime = 2000;

	return TrackMouseEvent(&tme);
}



void processaEventoRato(HWND hWnd, DadosThreadPipe* dados, int posX, int posY, int tipo) {
	//tipo -> 1 = clique esq;2 = clique dir;3 = mouse hover

	RECT rect;
	HDC hdc = GetDC(hWnd);
	GetClientRect(hWnd, &rect);
	int paddingX, paddingY, larguraSeq;
	WaitForSingleObject(dados->hMutex, INFINITE);
	int tamCelula = getPaddings(dados->tamX, dados->tamY, &rect, &paddingX, &paddingY, &larguraSeq, NULL);

	if (posX < paddingX + larguraSeq || posY < paddingY)
		return;
	if (posX > (dados->tamX * tamCelula) + paddingX + larguraSeq || posY > (dados->tamY * tamCelula) + paddingY) {
		return;
	}
	ReleaseMutex(dados->hMutex);

	int coordX, coordY;
	coordX = (posX - paddingX - larguraSeq) / tamCelula;
	coordY = (posY - paddingY) / tamCelula;

	//DEBUG START
	TCHAR a[512];
	_stprintf_s(a, 512, _T("Evento na célula %d, %d\n"), coordX, coordY);
	OutputDebugString(a);

	switch (tipo)
	{
	case 1:
		WaitForSingleObject(dados->hMutex, INFINITE);
		_stprintf_s(a, MAX, _T("LCLICK %d %d\n"), coordX, coordY);
		_tcscpy_s(dados->mensagem, MAX, a);
		ReleaseMutex(dados->hMutex);
		SetEvent(dados->hEventoNamedPipe);
		break;
	case 2:
		WaitForSingleObject(dados->hMutex, INFINITE);
		_stprintf_s(a, MAX, _T("RCLICK %d %d\n"), coordX, coordY);
		_tcscpy_s(dados->mensagem, MAX, a);
		ReleaseMutex(dados->hMutex);
		SetEvent(dados->hEventoNamedPipe);
		break;
	case 3:
		OutputDebugString(_T("Hover\n"));
		if (coordX == dados->celulaAtivaX && coordY == dados->celulaAtivaY) {
			OutputDebugString(_T("Hover na Célula Ativa\n"));
			//chama função para enviar mensagem ao servidor a avisar do evento hover
			WaitForSingleObject(dados->hMutex, INFINITE);
			_tcscpy_s(dados->mensagem, MAX, _T("HOVER"));
			ReleaseMutex(dados->hMutex);
			SetEvent(dados->hEventoNamedPipe);
			// a receção de uma resposta por parte do servidor deve iniciar um evento de deteção de que mexeu o rato, para parar o hover
			// não deve ser feito aqui, mas sim na thread de leitura de pipes
		}
		else {			//se não for a célula correta, recomeça a deteção de hover
			if (iniciaDetecaoHover(hWnd)) {
				OutputDebugString(_T("INICIOU DETEÇÂO HOVER\n"));
			}
			else {
				OutputDebugString(_T("NÃO CONSEGUIU INICIAR DETEÇÂO HOVER\n"));
			}
		}
		break;
	}
}



LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;
	static TCHAR lastChar = '?';
	static PosChar posicoes[1000];
	static DWORD count = 0;
	static HBITMAP hBmp[7];
	static int xBitmap, yBitmap;
	static BitmapInfo bitmap[NUM_BITMAPS];
	static HDC bmpDC = NULL;
	MINMAXINFO* mmi;
	static int xpto = 0;
	static int limDir;
	static HDC memDC = NULL;
	static DadosThreadPipe dados;
	HANDLE hThreadLer, hThreadEscrever;


	switch (messg) {
	case WM_CREATE:
		dados.terminar = 0;
		dados.hWnd = hWnd;

		hThreadLer = CreateThread(NULL, 0, ThreadLer, &dados, 0, NULL);
		if (hThreadLer == NULL) {
			OutputDebugString(TEXT("[Erro] ao criar thread!\n"));
			return -1;
		}
		dados.hThreadLer = hThreadLer;


		if (!initNamedPipes(&dados)) {
			OutputDebugString(_T("Erro ao criar named Pipes do Cliente.\n"));
			CloseHandle(hThreadLer);
			exit(1);
		}

		//criacao da thread
		hThreadEscrever = CreateThread(NULL, 0, ThreadEscrever, &dados, 0, NULL);
		if (hThreadEscrever == NULL) {
			OutputDebugString(TEXT("[Erro] ao criar thread!\n"));
			//closehandle
			return -1;
		}
		dados.hThreadEscrever = hThreadEscrever;


		loadImages(TRUE, bitmap, hWnd);

		//CARREGAR MAPA PRE-DEFINIDO PARA TESTE
		for (int x = 0; x < 20; x++){
			for (int y = 0; y < 20; y++)
			{
				dados.tabuleiro[x][y] = 0;
			}
		}
		dados.tabuleiro[0][2] = -1;
		dados.tabuleiro[1][2] = 1;
		dados.tabuleiro[2][2] = 1;
		dados.tabuleiro[3][2] = 1;
		dados.tabuleiro[4][2] = 4;
		dados.tabuleiro[4][3] = 2;
		dados.tabuleiro[4][4] = 5;
		dados.tabuleiro[3][4] = 3;
		dados.tabuleiro[3][5] = 2;
		dados.tabuleiro[3][6] = 6;
		dados.tabuleiro[4][6] = 1;
		dados.tabuleiro[5][6] = 1;
		dados.tabuleiro[6][6] = 1;
		dados.tabuleiro[7][6] = 1;
		dados.tabuleiro[8][6] = 5;
		dados.tabuleiro[8][5] = 3;
		dados.tabuleiro[9][5] = 1;
		
		dados.tabuleiro[9][6] = 3;
		dados.tabuleiro[0][0] = 5;

		dados.tamX = 10;
		dados.tamY = 7;

		for (int i = 0; i < 6; i++)
			dados.seq[i] = i;

		dados.celulaAtivaX = 0;
		dados.celulaAtivaY = 0;

		_stprintf_s(&dados.mensagem, MAX, _T("Isto é uma mensagem pré-definida, apenas para teste\0"));



		//GetObject(hBmp, sizeof(bmp), &bmp);
		hdc = GetDC(hWnd);
		ReleaseDC(hWnd, hdc);
		GetClientRect(hWnd, &rect);
		break;
	case WM_CLOSE:		// Close da janela	
		if (MessageBox(hWnd, _T("Queres mesmo sair?"), _T("SAIR"),
			MB_ICONQUESTION | MB_YESNO | MB_HELP) == IDYES)
		{
			//mandar mensagem ao sv a dizer que cliente saiu
			//escrever a msg
			WaitForSingleObject(dados.hMutex, INFINITE);
				_tcscpy_s(dados.mensagem,MAX, _T("SAIRCLI"));
			ReleaseMutex(dados.hMutex);
			SetEvent(dados.hEventoNamedPipe);
			CloseHandle(dados.hThreadEscrever);
			CloseHandle(dados.hThreadLer);
			DestroyWindow(hWnd);
		}
		break;
	case WM_HELP:		//Janela de Ajuda
		MessageBox(hWnd, _T("Janela de Ajuda"), _T("SAIR"), MB_OK);
		break;

	case WM_LBUTTONDOWN:	//apanhar evento que escuta tecla rato	
		processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),1);
		break;
	case WM_MOUSEHOVER:
		processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),3);
		break;
	case WM_PAINT:
		//Mostra no ecrã as informações do jogo
		hdc = BeginPaint(hWnd, &ps);
		atualizarDisplay(hWnd, &dados, bitmap);
		EndPaint(hWnd, &ps);
		break;
	case WM_RBUTTONDOWN:
		processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),2);
		break;
	case WM_CHAR:	//apanhar  teclado
		//PARA TESTE
		if (wParam == _T('d')){
			iniciaDetecaoHover(hWnd);
		}

		break;

	case WM_DESTROY:	//Destruir janela
		PostQuitMessage(0);
	case WM_ERASEBKGND: //Sent when the window background must be erased (for example, when a window is resized). 
		return 1;
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// n�o � efectuado nenhum processamento, apenas se segue o "default" do Windows
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;  // break tecnicamente desnecess�rio por causa do return
	}
	return(0);
}