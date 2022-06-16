#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#define TAM_BITMAP 150
#define NUM_BITMAPS 14
/* ===================================================== */
/* Programa base (esqueleto) para aplica��es Windows     */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
//  Composto por 2 fun��es: 
//	WinMain()     = Ponto de entrada dos programas windows
//			1) Define, cria e mostra a janela
//			2) Loop de recep��o de mensagens provenientes do Windows
//     TrataEventos()= Processamentos da janela (pode ter outro nome)
//			1) � chamada pelo Windows (callback) 
//			2) Executa c�digo em fun��o da mensagem recebida

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

// Nome da classe da janela (para programas de uma s� janela, normalmente este nome � 
// igual ao do pr�prio programa) "szprogName" � usado mais abaixo na defini��o das 
// propriedades do objecto janela
TCHAR szProgName[] = TEXT("Base2022");

// ============================================================================
// FUN��O DE IN�CIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa come�a sempre a sua execu��o na fun��o WinMain()que desempenha
// o papel da fun��o main() do C em modo consola WINAPI indica o "tipo da fun��o" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as fun��es de
// processamento da janela)
// Par�metros:
//   hInst: Gerado pelo Windows, � o handle (n�mero) da inst�ncia deste programa 
//   hPrevInst: Gerado pelo Windows, � sempre NULL para o NT (era usado no Windows 3.1)
//   lpCmdLine: Gerado pelo Windows, � um ponteiro para uma string terminada por 0
//              destinada a conter par�metros para o programa 
//   nCmdShow:  Par�metro que especifica o modo de exibi��o da janela (usado em  
//        	   ShowWindow()

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;		// hWnd � o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg;		// MSG � uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;	// WNDCLASSEX � uma estrutura cujos membros servem para 
			  // definir as caracter�sticas da classe da janela

	// ============================================================================
	// 1. Defini��o das caracter�sticas da janela "wcApp" 
	//    (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
	// ============================================================================
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
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo ser� branco

	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);

	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
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
	// ============================================================================
	// 5. Loop de Mensagens
	// ============================================================================
	// O Windows envia mensagens �s janelas (programas). Estas mensagens ficam numa fila de
	// espera at� que GetMessage(...) possa ler "a mensagem seguinte"	
	// Par�metros de "getMessage":
	// 1)"&lpMsg"=Endere�o de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
	//   in�cio de WinMain()):
	//			HWND hwnd		handler da janela a que se destina a mensagem
	//			UINT message		Identificador da mensagem
	//			WPARAM wParam		Par�metro, p.e. c�digo da tecla premida
	//			LPARAM lParam		Par�metro, p.e. se ALT tamb�m estava premida
	//			DWORD time		Hora a que a mensagem foi enviada pelo Windows
	//			POINT pt		Localiza��o do mouse (x, y) 
	// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
	//   receber as mensagens para todas as
	// janelas pertencentes � thread actual)
	// 3)C�digo limite inferior das mensagens que se pretendem receber
	// 4)C�digo limite superior das mensagens que se pretendem receber

	// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
	// 	  terminando ent�o o loop de recep��o de mensagens, e o programa 

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);	// Pr�-processamento da mensagem (p.e. obter c�digo 
					   // ASCII da tecla premida)
		DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
					   // aguarda at� que a possa reenviar � fun��o de 
					   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam);	// Retorna sempre o par�metro wParam da estrutura lpMsg
}

// ============================================================================
// FUN��O DE PROCESSAMENTO DA JANELA
// Esta fun��o pode ter um nome qualquer: Apenas � neces�rio que na inicializa��o da
// estrutura "wcApp", feita no in�cio de // WinMain(), se identifique essa fun��o. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pr�-processadas
// no loop "while" da fun��o WinMain()
// Par�metros:
//		hWnd	O handler da janela, obtido no CreateWindow()
//		messg	Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
//		wParam	O par�metro wParam da estrutura messg (a mensagem)
//		lParam	O par�metro lParam desta mesma estrutura
//
// NOTA:Estes par�metros est�o aqui acess�veis o que simplifica o acesso aos seus valores
//
// A fun��o EndProc � sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar.
// Estas mensagens s�o identificadas por constantes (p.e. 
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
// ============================================================================

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

int getPaddings(int tamX, int tamY, RECT* rect, int* paddingX, int* paddingY) {			
	// Recebe dimensoes do tabuleiro, o rect do ecrã e ponteiros para as variáveis que guardaram os paddings
	// Devolve o tamanho de célula

	int tamCelula;
	*paddingX = rect->right / tamX;
	*paddingY = rect->bottom / tamY;

	if (*paddingX >= *paddingY) {
		tamCelula = *paddingY;
		*paddingX = (rect->right - tamCelula * tamX) / 2;
		*paddingY = 0;
	}
	else {
		tamCelula = *paddingX;
		*paddingY = (rect->bottom - tamCelula * tamY) / 2;
		*paddingX = 0;
	}

	return tamCelula;
}

void imprimirTabuleiro(HWND hWnd, int tabuleiro[20][20], int tamX, int tamY, BitmapInfo bitmap[]) {
	RECT rect;
	HDC hdc = GetDC(hWnd);
	GetClientRect(hWnd, &rect);
	int paddingX, paddingY;
	int tamCelula = getPaddings(tamX, tamY, &rect, &paddingX, &paddingY);

	BITMAP currBitmap;


	FillRect(hdc, &rect, CreateSolidBrush(RGB(100, 100, 100)));
	for (int x = 0; x < tamX; x++){
		for (int y = 0; y < tamY; y++){	
			currBitmap = bitmap[tabuleiro[x][y] + 6].bmp;
			StretchBlt(hdc, (tamCelula * x) + paddingX, (tamCelula * y) + paddingY, tamCelula, tamCelula, bitmap[tabuleiro[x][y] + 6].bmpDC, 0, 0, currBitmap.bmWidth, currBitmap.bmHeight, SRCCOPY);
			rect.left = (tamCelula * x) + paddingX;
			rect.top = (tamCelula * y) + paddingY;
			rect.bottom = rect.top + tamCelula;
			rect.right = rect.left + tamCelula;
		}
	}
}

void processaClique(HWND hWnd, int tamX, int tamY, int posCliqueX, int posCliqueY) {
	RECT rect;
	HDC hdc = GetDC(hWnd);
	GetClientRect(hWnd, &rect);
	int paddingX, paddingY;
	int tamCelula = getPaddings(tamX, tamY, &rect, &paddingX, &paddingY);

	if (posCliqueX < paddingX || posCliqueY < paddingY)
		return;
	if (posCliqueX > (tamX * tamCelula) + paddingX || posCliqueY > (tamY * tamCelula) + paddingY){
		return;
	}
	int coordX, coordY;
	coordX = (posCliqueX - paddingX) / tamCelula;
	coordY = (posCliqueY - paddingY) / tamCelula;

	//DEBUG START
	TCHAR a[512];
	_stprintf_s(a, 512, _T("TAM_CELULA = %d ,Clique %d, %d, Celula %d, %d\n"), tamCelula, posCliqueX, posCliqueY, coordX, coordY);
	OutputDebugString(a);
	//DEBUG END

	//aqui chama a função para enviar mensagem ao servidor, para avisar do clique
}

BOOL iniciaDetecaoHover(HWND hWnd) {
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_HOVER;
	tme.hwndTrack = hWnd;
	tme.dwHoverTime = 2000;

	return TrackMouseEvent(&tme);
}

void processaHover(HWND hWnd, int tamX, int tamY, int posHoverX, int posHoverY) {
	RECT rect;
	HDC hdc = GetDC(hWnd);
	GetClientRect(hWnd, &rect);
	int paddingX, paddingY;
	int tamCelula = getPaddings(tamX, tamY, &rect, &paddingX, &paddingY);

	if (posHoverX < paddingX || posHoverY < paddingY)
		return;
	if (posHoverX > (tamX * tamCelula) + paddingX || posHoverY > (tamY * tamCelula) + paddingY) {
		return;
	}
	int coordX, coordY;
	coordX = (posHoverX - paddingX) / tamCelula;
	coordY = (posHoverY - paddingY) / tamCelula;

	//DEBUG START
	TCHAR a[512];
	_stprintf_s(a, 512, _T("Hovered célula %d, %d\n"), coordX, coordY);
	OutputDebugString(a);
	//DEBUG END

	int celulaAtivaX = 0, celulaAtivaY = 0;		//apenas aqui para referência futura. estes dados farão parte de uma estrutura que guarde também o tabuleiro e as suas dimensoes
	if (coordX == celulaAtivaX && coordY == celulaAtivaY){
		OutputDebugString(_T("Hover na Célula Ativa\n"));
		//chama função para enviar mensagem ao servidor a avisar do evento hover
		
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
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;
	static TCHAR lastChar = '?';
	static TCHAR c;
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
	static int tabuleiro[20][20];
	static int tamX = 10, tamY = 7;

	switch (messg) {
	case WM_CREATE:
		loadImages(TRUE, bitmap, hWnd);

		//CARREGAR MAPA PRE-DEFINIDO PARA TESTE
		for (int x = 0; x < 20; x++){
			for (int y = 0; y < 20; y++)
			{
				tabuleiro[x][y] = 0;
			}
		}
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
		
		tabuleiro[9][6] = 3;
		tabuleiro[0][0] = 5;

		//GetObject(hBmp, sizeof(bmp), &bmp);
		hdc = GetDC(hWnd);
		ReleaseDC(hWnd, hdc);
		GetClientRect(hWnd, &rect);
		break;
	case WM_CLOSE:		// Close da janela	
		if (MessageBox(hWnd, _T("Queres mesmo sair?"), _T("SAIR"),
			MB_ICONQUESTION | MB_YESNO | MB_HELP) == IDYES)
		{
			DestroyWindow(hWnd);
		}
		break;
	case WM_HELP:		//Janela de Ajuda
		MessageBox(hWnd, _T("Janela de Ajuda"), _T("SAIR"), MB_OK);
		break;

	case WM_LBUTTONDOWN:	//apanhar evento que escuta tecla rato	
		processaClique(hWnd, tamX, tamY, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		//InvalidateRect(hWnd, NULL, TRUE);		//Chama WM_PAINT
		
		// APENAS PARA TESTE. SERÁ REMOVIDO DAQUI E CHAMADO QUANDO RECEBER INFORMAÇÃO DO SERVIDOR DE QUE SE INICIOU O JOGO
		iniciaDetecaoHover(hWnd);
		
		break;
	case WM_MOUSEHOVER:
		processaHover(hWnd, tamX, tamY, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_PAINT:
		
		hdc = BeginPaint(hWnd, &ps);

		imprimirTabuleiro(hWnd, tabuleiro, tamX, tamY, bitmap);

		EndPaint(hWnd, &ps);
		break;
	case WM_CHAR:	//apanhar  teclado
		c = (wchar_t)wParam;
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