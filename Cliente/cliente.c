#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "Cliente.h"
#include "resource.h"

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM LParam);
BOOL CALLBACK DlgProc2(HWND dlg, UINT msg, WPARAM wParam, LPARAM LParam);
TCHAR szProgName[] = TEXT("Base2022");

BOOL CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM LParam) {
	DATA* pData = (DATA*)GetWindowLongPtr(GetParent(hWnd), 0);
	switch (msg) {
	case WM_INITDIALOG:
		SetDlgItemText(hWnd, IDC_EDIT1, pData->nome);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			GetDlgItemText(hWnd, IDC_EDIT1, pData->nome, 80);
			//OutputDebugString(TEXT("nome mudado!\n"));
			EndDialog(hWnd, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		default:
			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hWnd, (INT_PTR)0);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK DlgProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM LParam) {
	switch (msg) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			EndDialog(hWnd, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		default:
			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hWnd, (INT_PTR)0);
		return TRUE;
	}
	return FALSE;
}



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


int atoicmd(TCHAR** cmd, int i) {
	int atoi;
	if (_tcscmp(cmd[i], _T("0")) != 0) {		//verifica se valor não é igual a '0' pois atoi devolve 0 quando é erro
		atoi = _tstoi(cmd[i]);
		if (atoi == 0) {
			_tprintf(_T("Valor passado como argumento não é aceite\n"));
			return -1000;		
		}
		return atoi;
	}
	else
		return 0;
}


/*****************************************/
/***************ThreadLer*****************/
/*****************************************/
DWORD WINAPI ThreadLer(LPVOID param) {						
	DadosThreadPipe* dados = (DadosThreadPipe*)param;
	TCHAR buf[MAX], ** arrayComandos = NULL;
	DWORD n;
	unsigned int nrArgs = 0;
	const TCHAR delim[2] = _T(" ");
	TCHAR a[MAX];
	HANDLE hPipe;

	OutputDebugString(TEXT("[Cliente] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);

	//bloqueia à espera que exista um named pipe para ler do mesmo
	if (!WaitNamedPipe(NOME_PIPE_SERVIDOR, NMPWAIT_WAIT_FOREVER)) {
		OutputDebugString(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	OutputDebugString(TEXT("[Cliente] Ligação ao pipe do servidor... (CreateFile)\n"));

	hPipe = CreateFile(NOME_PIPE_SERVIDOR, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hPipe == NULL) {
		OutputDebugString(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	OutputDebugString(TEXT("[Cliente] Liguei-me...\n"));


	while (dados->terminar == 0) {
		//le as mensagens enviadas pelo cliente
		BOOL ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
		if (n == 0) {
			continue;
		}

		buf[n / sizeof(TCHAR) - 1] = '\0';

		if (!ret || !n) {
			OutputDebugString(TEXT("[Cliente] %d %d... (ReadFile)\n"), ret, n);
			break;
		}

		_stprintf_s(a, MAX, TEXT("[Cliente] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
		OutputDebugString(a);
		
		arrayComandos = divideString(buf, delim, &nrArgs);			//divisão da string para um array com o comando e args


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (_tcscmp(arrayComandos[0], INICIAJOGO) == 0) {	//msg: iniciajogo peçainicialx pecainicialy tipopeçainicial peçafinalx pecafinaly tipopeçafinal
			if (nrArgs >= 6) {
				unsigned int initx, inity, pecai, fimx, fimy, pecaf;
				initx = atoicmd(arrayComandos, 1);
				inity = atoicmd(arrayComandos, 2);
				fimx = atoicmd(arrayComandos, 4);
				fimy = atoicmd(arrayComandos, 5);
				pecai = atoicmd(arrayComandos,3);
				pecaf = atoicmd(arrayComandos,6);

				dados->tabuleiro[initx][inity] = pecai;
				dados->tabuleiro[fimx][fimy] = pecaf;

				dados->jogoCorrer = TRUE;

				InvalidateRect(dados->hWnd, NULL, TRUE);        //Chama WM_PAINT
			}
		}else if (_tcscmp(arrayComandos[0], JOGOMULTIPCANCEL) == 0) {
			//TODO
		}
		else if (_tcscmp(arrayComandos[0], INFO) == 0) {		//msg: info msgcominformação
			_tcscpy_s(dados->info, MAX, arrayComandos[1]);
			InvalidateRect(dados->hWnd, NULL, TRUE);       

		}
		else if (_tcscmp(arrayComandos[0], PECA) == 0) {		//msg: peca x y tipo_de_peca
			if (nrArgs >= 3) {	//x,y,tipopeça
				unsigned int x, y, t;
				x = atoicmd(arrayComandos, 1);
				y = atoicmd(arrayComandos, 2);
				t = atoicmd(arrayComandos, 3);

				WaitForSingleObject(dados->hMutex, INFINITE);
				dados->tabuleiro[x][y] = t;
				ReleaseMutex(dados->hMutex);
				InvalidateRect(dados->hWnd, NULL, TRUE);       
			}
		}
		else if (_tcscmp(arrayComandos[0], SEQ) == 0) {			//msg: seq tipopeça1 tipopeça2 ...
			if (nrArgs >= 6) {
				for (int i = 1; i < 7; i++)
				{
					int t = 0;
					t = atoicmd(arrayComandos, i);

					WaitForSingleObject(dados->hMutex, INFINITE);
					dados->seq[i-1] = t;
					ReleaseMutex(dados->hMutex);
					InvalidateRect(dados->hWnd, NULL, TRUE);      
				}	
			}
		}
		else if (_tcscmp(arrayComandos[0], GANHOU) == 0) {
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG3), dados->hWnd, DlgProc2);

			//talvez limpar mapa
		}
		else if (_tcscmp(arrayComandos[0], PERDEU) == 0) {
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG4), dados->hWnd, DlgProc2);
			//talvez limpar mapa
		}
		else if (_tcscmp(arrayComandos[0], SUSPENDE) == 0) {
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG5), dados->hWnd, DlgProc2);

		}
		else if (_tcscmp(arrayComandos[0], RETOMA) == 0) {
			//desativa a textbox e continua o jogo

		}
		else if (_tcscmp(arrayComandos[0], SAIR) == 0) {
			dados->terminar = 1;
			sair(dados);
			//fechar tudoooo
		}

	}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
			else {
				TCHAR a[MAX];
				_stprintf_s(a, MAX, _T("Enviei %d bytes ao cliente\n %s"), n, dados->mensagem);
				OutputDebugString(a);

			}
		}
		ReleaseMutex(dados->hMutex);
		ResetEvent(dados->hEventoNamedPipe);

	} while (dados->terminar == 0);
	return 0;
}


BOOL initNamedPipes(DadosThreadPipe* dados) {
	HANDLE hPipe, hThread;
	dados->terminar = 0;
	TCHAR nomeMutex[MAX];

	_stprintf_s(nomeMutex, MAX, _T("MUTEX_NPIPE_CLI%d\0"), GetProcessId(GetCurrentProcess()));
	OutputDebugString(nomeMutex);

	dados->hMutex = CreateMutex(NULL, FALSE, nomeMutex); //Criação do mutex
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

	dados->hEventoNamedPipe = CreateEvent(NULL, TRUE, FALSE, NULL);
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
	DATA dados = { _T("Jogador1") };

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
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);;			// Classe do menu que a janela pode ter
							  // (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0;				// Livre, para uso particular
	wcApp.cbWndExtra = sizeof(DATA*);				// Livre, para uso particular
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

	SetWindowLongPtr(hWnd, 0, (LONG_PTR)&dados);
	
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

void loadImages(HANDLE mutexBitmap ,BOOL texturas, BitmapInfo bitmap[], HWND hWnd) {
	HBITMAP hBmp;
	HDC hdc = GetDC(hWnd);

	WaitForSingleObject(mutexBitmap, INFINITE);

	if (texturas){
		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe6_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[0].bmp);
		bitmap[0].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[0].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe5_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[1].bmp);
		bitmap[1].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[1].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe4_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[2].bmp);
		bitmap[2].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[2].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe3_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[3].bmp);
		bitmap[3].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[3].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe2_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[4].bmp);
		bitmap[4].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[4].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe1_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[5].bmp);
		bitmap[5].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[5].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe0.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[6].bmp);
		bitmap[6].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[6].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe1.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[7].bmp);
		bitmap[7].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[7].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe2.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[8].bmp);
		bitmap[8].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[8].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe3.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[9].bmp);
		bitmap[9].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[9].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe4.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[10].bmp);
		bitmap[10].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[10].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe5.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[11].bmp);
		bitmap[11].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[11].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe6.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[12].bmp);
		bitmap[12].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[12].bmpDC, hBmp);

		hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_moderno\\pipe7.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(BITMAP), &bitmap[13].bmp);
		bitmap[13].bmpDC = CreateCompatibleDC(hdc);
		SelectObject(bitmap[13].bmpDC, hBmp);

		ReleaseMutex(mutexBitmap);
		return;
	}

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe6_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[0].bmp);
	bitmap[0].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[0].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe5_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[1].bmp);
	bitmap[1].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[1].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe4_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[2].bmp);
	bitmap[2].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[2].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe3_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[3].bmp);
	bitmap[3].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[3].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe2_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[4].bmp);
	bitmap[4].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[4].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe1_agua.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[5].bmp);
	bitmap[5].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[5].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe0.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[6].bmp);
	bitmap[6].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[6].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe1.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[7].bmp);
	bitmap[7].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[7].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe2.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[8].bmp);
	bitmap[8].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[8].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe3.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[9].bmp);
	bitmap[9].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[9].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe4.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[10].bmp);
	bitmap[10].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[10].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe5.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[11].bmp);
	bitmap[11].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[11].bmpDC, hBmp);

	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe6.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[12].bmp);
	bitmap[12].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[12].bmpDC, hBmp);


	hBmp = (HBITMAP)LoadImage(NULL, _T("..\\Imagens\\texture_pack_retro\\pipe7.bmp"), IMAGE_BITMAP, TAM_BITMAP, TAM_BITMAP, LR_LOADFROMFILE);
	GetObject(hBmp, sizeof(BITMAP), &bitmap[13].bmp);
	bitmap[13].bmpDC = CreateCompatibleDC(hdc);
	SelectObject(bitmap[13].bmpDC, hBmp);

	ReleaseMutex(mutexBitmap);
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
	
	WaitForSingleObject(dados->hMutexBitmap, INFINITE);
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
	ReleaseMutex(dados->hMutexBitmap);

	//PRINT INFO
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, TRANSPARENT);
	rect.left = 0;
	rect.top = 0;
	DrawText(hdc, &dados->info, -1, &rect, DT_SINGLELINE, DT_NOCLIP);
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
	//WaitForSingleObject(dados->hMutex, INFINITE);
	int tamCelula = getPaddings(dados->tamX, dados->tamY, &rect, &paddingX, &paddingY, &larguraSeq, NULL);

	if (posX < paddingX + larguraSeq || posY < paddingY + ALTURA_INFO)
		return;
	if (posX > (dados->tamX * tamCelula) + paddingX + larguraSeq || posY > (dados->tamY * tamCelula) + paddingY + ALTURA_INFO) {
		return;
	}
	//ReleaseMutex(dados->hMutex);

	int coordX, coordY;
	coordX = (posX - paddingX - larguraSeq) / tamCelula;
	coordY = (posY - paddingY - ALTURA_INFO) / tamCelula;

	if (coordX >= dados->tamX || coordY>=dados->tamY){
		return;
	}

	//DEBUG START
	TCHAR a[512];
	_stprintf_s(a, 512, _T("Evento na célula %d, %d\n"), coordX, coordY);
	OutputDebugString(a);

	switch (tipo)
	{
	case 1:
		WaitForSingleObject(dados->hMutex, INFINITE);
		_stprintf_s(a, MAX, _T("LCLICK %d %d \n"), coordX, coordY);
		_tcscpy_s(dados->mensagem, MAX, a);
		ReleaseMutex(dados->hMutex);
		SetEvent(dados->hEventoNamedPipe);
		break;
	case 2:
		WaitForSingleObject(dados->hMutex, INFINITE);
		_stprintf_s(a, MAX, _T("RCLICK %d %d \n"), coordX, coordY);
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
			ReleaseMutex(dados->hMutex);
		}
		break;
	default:
		ReleaseMutex(dados->hMutex);
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
	DATA* dadosDentroJogo = (LONG_PTR)GetWindowLongPtr(hWnd, 0);
	TCHAR a[MAX];


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

		TCHAR nomeMutexBitmap[MAX];

		_stprintf_s(nomeMutexBitmap, MAX, _T("MUTEX_BITMAP%d\0"), GetProcessId(GetCurrentProcess()));
		MUTEX_BITMAP;
		dados.hMutexBitmap = CreateMutex(NULL, FALSE, nomeMutexBitmap);


		loadImages(dados.hMutexBitmap, dados.texturas, bitmap, hWnd);

		////CARREGAR MAPA PRE-DEFINIDO PARA TESTE
		for (int x = 0; x < 20; x++){
			for (int y = 0; y < 20; y++)
			{
				dados.tabuleiro[x][y] = 0;
			}
		}
		//dados.tabuleiro[3][2] = 7;
		/*dados.tabuleiro[0][2] = -1;
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
		dados.tabuleiro[0][0] = 5;*/

		dados.tamX = 10;
		dados.tamY = 7;

		for (int i = 0; i < 6; i++)
			dados.seq[i] = i+2;

		dados.celulaAtivaX = 0;
		dados.celulaAtivaY = 0;

		dados.jogoCorrer = FALSE;

		_stprintf_s(&dados.mensagem, MAX, _T("Isto é uma mensagem pré-definida, apenas para teste\0"));



		//GetObject(hBmp, sizeof(bmp), &bmp);
		hdc = GetDC(hWnd);
		ReleaseDC(hWnd, hdc);
		GetClientRect(hWnd, &rect);
		break;
	case WM_CLOSE:		// Close da janela	
		if (MessageBox(hWnd, _T("Queres mesmo sair?"), _T("SAIR"),
			MB_ICONQUESTION | MB_YESNO) == IDYES)
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

	case WM_LBUTTONDOWN:	//apanhar evento que escuta tecla rato	
		if (dados.jogoCorrer){
			processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 1);
		}
		break;
	case WM_RBUTTONDOWN:
		if (dados.jogoCorrer) {
			processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 2);
		}
		break;
	case WM_MOUSEHOVER:
		if (dados.jogoCorrer) {
			processaEventoRato(hWnd, &dados, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 3);
		}
		break;
	case WM_PAINT:
		//Mostra no ecrã as informações do jogo
		hdc = BeginPaint(hWnd, &ps);
		atualizarDisplay(hWnd, &dados, bitmap);
		EndPaint(hWnd, &ps);
		break;
	case WM_CHAR:	//apanhar  teclado
		//PARA TESTE
		if (wParam == _T('d')){
			iniciaDetecaoHover(hWnd);
		}
		if (wParam == _T('s')){
			dados.jogoCorrer = TRUE;
		}

		break;

	case WM_DESTROY:	//Destruir janela
		PostQuitMessage(0);
	case WM_ERASEBKGND: //Sent when the window background must be erased (for example, when a window is resized). 
		return 1;
	case WM_COMMAND: //menu
		switch (wParam)
		{
		case ID_MENU_NOME:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgProc);
			break;
		case ID_MENU_SINGLEPLAYER:
			WaitForSingleObject(dados.hMutex, INFINITE);
			_stprintf_s(a, MAX, _T("JOGOSINGLEP %s"), dadosDentroJogo->nome);
			_tcscpy_s(dados.mensagem, MAX, a);
			ReleaseMutex(dados.hMutex);
			SetEvent(dados.hEventoNamedPipe);
			break;
		case ID_MENU_MULTIPLAYER:
			WaitForSingleObject(dados.hMutex, INFINITE);
			_stprintf_s(a, MAX, _T("JOGOMULTIP %s"), dadosDentroJogo->nome);
			_tcscpy_s(dados.mensagem, MAX, a);
			ReleaseMutex(dados.hMutex);
			SetEvent(dados.hEventoNamedPipe);
			//ativar Dialog2
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG2), dados.hWnd, DlgProc2);
			//OutputDebugString(dados.dialogEspera);
			break;
		case ID_MENU_CANCELMULTIPLAYER:
			WaitForSingleObject(dados.hMutex, INFINITE);
			_tcscpy_s(dados.mensagem, MAX, JOGOMULTIPCANCEL);
			ReleaseMutex(dados.hMutex);
			SetEvent(dados.hEventoNamedPipe);
			break;
		case ID_MENU_ALTERATEXTURA:
			dados.texturas = (!dados.texturas);
			loadImages(dados.hMutexBitmap, dados.texturas, bitmap, hWnd);
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		default:
			break;
		}
	default:
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;  // break tecnicamente desnecess�rio por causa do return
	}
	return(0);
}


int sair(DadosThreadPipe* dados) {
	CloseHandle(dados->hEventoNamedPipe);
	CloseHandle(dados->hMutex);
	CloseHandle(dados->hMutexBitmap);
	CloseHandle(dados->hThreadEscrever);
	CloseHandle(dados->hThreadLer);
	DisconnectNamedPipe(dados->hPipe.hPipe);
	PostQuitMessage(0);
}