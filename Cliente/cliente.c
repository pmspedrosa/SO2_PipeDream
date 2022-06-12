#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "cliente.h"

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





DWORD WINAPI ThreadLer(LPVOID param) {						
	//fica sempre � escuta de ler coisas vindas do named pipe
	//escreve diretamente no ecr�
	DadosThreadPipes* dados = (DadosThreadPipes*)param;
	TCHAR buf[MAX];
	DWORD n;

	_tprintf(TEXT("[Cliente] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), NOME_PIPE_CLIENTE);

	//espera que exista um named pipe para ler do mesmo
	//bloqueia aqui
	if (!WaitNamedPipe(NOME_PIPE_CLIENTE, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}

	_tprintf(TEXT("[Cliente] Liga��o ao pipe do cliente... (CreateFile)\n"));

	HANDLE hPipe = CreateFile(NOME_PIPE_CLIENTE, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hPipe == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NOME_PIPE_CLIENTE);
		exit(-1);
	}

	_tprintf(TEXT("[Cliente] Liguei-me...\n"));

	while (dados->terminar == 0) {
		//le as mensagens enviadas pelo cliente
		BOOL ret = ReadFile(hPipe, &buf, sizeof(buf), &n, NULL);



		if (!ret || !n) {
			_tprintf(TEXT("[Cliente] %d %d... (ReadFile)\n"), ret, n);
			break;
		}

		_tprintf(TEXT("[Cliente] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);

		
	}

	CloseHandle(hPipe);
	Sleep(200);

	return 0;

}



DWORD WINAPI ThreadEscrever(LPVOID param) {								//thread escritura de informa��es para o cliente atrav�s do named pipe
	//funcionamento -> evento ativa quando texto vari�vel de "escrita" � alterada
	DadosThreadPipes* dados = (DadosThreadPipes*)param;
	Msg *buf;
	DWORD n;
	int i;

	do {
		//ficar bloqueado � espera de um evento qualquer
		//l� o comando enviado de algum lado, deve ser dos eventos do rato,etc

		//escreve no named pipe
		for (i = 0; i < NPIPES; i++) {
			WaitForSingleObject(dados->hMutex, INFINITE);
			if (dados->hPipe.activo) {
				if (!WriteFile(dados->hPipe.hPipe, buf, _tcslen(buf) * sizeof(TCHAR), &n, NULL))
					_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
				else
					_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor [%d]... (WriteFile)\n"), n, i);
			}
			//libertamos o mutex
			ReleaseMutex(dados->hMutex);
		}

	} while (dados->terminar == 0);
	dados->terminar = 1;

	for (i = 0; i < NPIPES; i++)
		SetEvent(dados->hEvent);

	return 0;
}




BOOL initNamedPipes(DadosThreadPipes* dados) {
	//FILE_FLAG_OVERLAPPED para o named pipe aceitar comunica��es assincronas
	HANDLE hPipe = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, NPIPES, MAX * sizeof(TCHAR),
		MAX * sizeof(TCHAR), 1000, NULL);

	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
		exit(-1);
	}

	dados->hPipe.hPipe = hPipe;
	dados->terminar = 0;

	// criar evento que vai ser associado � estrutura overlaped
	// os eventos aqui tem de ter sempre reset manual e nao autom�tico porque temos de delegar essas responsabilidades ao sistema operativo
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (hEvent == NULL) {
		_tprintf(TEXT("[ERRO] ao criar evento\n"));
		dados->terminar = 1;
		return -1;
	}

	dados->hPipe.hPipe = hPipe;
	dados->hPipe.activo = FALSE;
	//temos de garantir que a estrutura overlap est� limpa
	ZeroMemory(&dados->hPipe.overlap, sizeof(dados->hPipe.overlap));
	//preenchemos agora o evento
	dados->hPipe.overlap.hEvent = hEvent;
	dados->hEvent = hEvent;

	_tprintf(TEXT("[Cliente] Esperar liga��o de um servidor... (ConnectNamedPipe)\n"));

	// aqui passamos um ponteiro para a estrutura overlap
	ConnectNamedPipe(hPipe, &dados->hPipe.overlap);

	//criacao da thread
	HANDLE hThread = CreateThread(NULL, 0, ThreadEscrever, &dados, 0, NULL);
	if (hThread == NULL) {
		_tprintf(TEXT("[Erro] ao criar thread!\n"));
		return -1;
	}
}







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

	//utilizar a teentativa de abrir o evento como forma de saber se o servidor est� ativo
	HANDLE hEventUpdateTabuleiro = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TABULEIRO);

	if (hEventUpdateTabuleiro == NULL) {
		_tprintf(_T("Erro: OpenEvent (%d)\n"), GetLastError());
		//dados->terminar = 1;
		return 0;
	}

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

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	RECT rect;
	static TCHAR c = '?';
	int xPos, yPos;

	switch (messg) {
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
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		hdc = GetDC(hWnd);		//a fun��o GetDC recupera um identificador para um contexto de dispositivo (DC) ...
		GetClientRect(hWnd, &rect);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		rect.left = xPos;
		rect.top = yPos;
		DrawText(hdc, &c, 1, &rect, DT_SINGLELINE | DT_NOCLIP);
		ReleaseDC(hWnd, hdc);
		break;
	case WM_CHAR:	//apanhar  teclado
		c = (wchar_t)wParam;
		break;

	case WM_DESTROY:	//Destruir janela
		PostQuitMessage(0);
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// n�o � efectuado nenhum processamento, apenas se segue o "default" do Windows
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;  // break tecnicamente desnecess�rio por causa do return
	}
	return(0);
}
