#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "cliente.h"

Msg msg;
TCHAR mensagem[MAX];
TCHAR cmd[MAX];

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base2022");



TCHAR comandosParaString(Msg msg) {
	TCHAR str[MAX] = _T("");
	TCHAR tk[MAX] = _T(" ");

	_tcscat_s(str, MAX, msg.cmd);
	if (msg.numargs > 0) {
		for (int i = 0; i < msg.numargs; i++)
		{
			_tcscat_s(str, MAX, tk);
			_tcscat_s(str, MAX, msg.args[i]);
		}
	}

	return str;
}



DWORD WINAPI ThreadLer(LPVOID param) {						
	//fica sempre á escuta de ler coisas vindas do named pipe
	//escreve diretamente no ecrã
	DadosThreadPipe* dados = (DadosThreadPipe*)param;
	TCHAR buf[MAX];
	DWORD n;

	_tprintf(TEXT("[Cliente] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);

	//espera que exista um named pipe para ler do mesmo
	//bloqueia aqui
	if (!WaitNamedPipe(NOME_PIPE_SERVIDOR, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	_tprintf(TEXT("[Cliente] Ligação ao pipe do servidor... (CreateFile)\n"));

	HANDLE hPipe = CreateFile(NOME_PIPE_SERVIDOR, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hPipe == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NOME_PIPE_SERVIDOR);
		exit(-1);
	}

	_tprintf(TEXT("[Cliente] Liguei-me...\n"));

	while (dados->terminar == 0) {
		//le as mensagens enviadas pelo cliente
		BOOL ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
		buf[n / sizeof(TCHAR)] = '\0';

		if (!ret || !n) {
			_tprintf(TEXT("[Cliente] %d %d... (ReadFile)\n"), ret, n);
			break;
		}

		_tprintf(TEXT("[Cliente] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
		_tcscpy_s(mensagem, MAX, buf);

	}

	CloseHandle(hPipe);
	Sleep(200);

	return 0;

}


DWORD WINAPI ThreadEscrever(LPVOID param) {								//thread escritura de informações para o servidor através do named pipe
	//funcionamento -> evento ativa quando texto variável de Msg é alterada
	DadosThreadPipe* dados = (DadosThreadPipe*)param;
	DWORD n;
	int i;


	do {
		//ficar bloqueado à espera de um evento qualquer
		
		//sistema de mensagens

		/*para->água parada
		pi-> impossivel colocar peça no local indicado
		pc-> peça colocada com sucesso
		perdeu -> perdeu
		ganhou -> ganhou
		barr -> barreira adicionada
		...
		*/

		//eventooooooooooooooooooooo



		Sleep(5000);
		TCHAR buf[MAX] = _T("asdilvbna");
		_tcscpy_s(cmd, MAX,buf);
		

		WaitForSingleObject(dados->hMutex, INFINITE);
		if (dados->hPipe.activo) {
			if (!WriteFile(dados->hPipe.hPipe, cmd, _tcslen(cmd) * sizeof(TCHAR), &n, NULL))
				_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			else
				_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao cliente ... (WriteFile)\n"), n);
		}
		ReleaseMutex(dados->hMutex);
		Sleep(2000);

	} while (dados->terminar == 0);
	CloseHandle(dados->hThread);
	return 0;
}


BOOL initNamedPipes(DadosThreadPipe* dados) {
	HANDLE hPipe, hThread;
	dados->terminar = 0;

	dados->hMutex = CreateMutex(NULL, FALSE, MUTEX_NPIPE_CLI); //Criação do mutex
	if (dados->hMutex == NULL) {
		_tprintf(TEXT("[Erro] ao criar mutex!\n"));
		return -1;
	}

	hPipe = CreateNamedPipe(NOME_PIPE_CLIENTE, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, MAX * sizeof(TCHAR),MAX * sizeof(TCHAR), 1000, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
		exit(-1);
	}

	WaitForSingleObject(dados->hMutex, INFINITE);
	dados->hPipe.hPipe = hPipe;
	dados->hPipe.activo = FALSE;
	ReleaseMutex(dados->hMutex);

	_tprintf(TEXT("[CLIENTE] Esperar ligação de um servidor... (ConnectNamedPipe)\n"));
	//o cliente espera até ter um servidor conectado a esta instância
	if (!ConnectNamedPipe(dados->hPipe.hPipe, NULL)) {
		_tprintf(TEXT("[ERRO] Ligação ao servidor! (ConnectNamedPipe\n"));
		exit(-1);
	}
	_tprintf(TEXT(" Ligação ao servidor com sucesso! (ConnectNamedPipe\n"));

	WaitForSingleObject(dados->hMutex, INFINITE);
	dados->hPipe.activo = TRUE;
	ReleaseMutex(dados->hMutex);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;		// hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg;		// MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;	// WNDCLASSEX é uma estrutura cujos membros servem para 
			  // definir as características da classe da janela

	DadosThreadPipe dadosPipes;

	//utilizar a teentativa de abrir o evento como forma de saber se o servidor está ativo
	HANDLE hEventUpdateTabuleiro = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TABULEIRO);

	if (hEventUpdateTabuleiro == NULL) {
		_tprintf(_T("Erro: OpenEvent (%d)\n"), GetLastError());
		//dados->terminar = 1;
		return 0;
	}

	dadosPipes.terminar = 0;

	HANDLE hThreadLer = CreateThread(NULL, 0, ThreadLer, &dadosPipes, 0, NULL);
	if (hThreadLer == NULL) {
		_tprintf(TEXT("[Erro] ao criar thread!\n"));
		return -1;
	}


	if (!initNamedPipes(&dadosPipes)) {
		_tprintf(_T("Erro ao criar named Pipes do Cliente.\n"));;
		exit(1);
	}

	//criacao da thread
	HANDLE hThreadEscrever = CreateThread(NULL, 0, ThreadEscrever, &dadosPipes, 0, NULL);
	if (hThreadEscrever == NULL) {
		_tprintf(TEXT("[Erro] ao criar thread!\n"));
		return -1;
	}
	
	

	//dadosPipes.hThread = hThreadEscrever;
	



	// ============================================================================
	// 1. Definição das características da janela "wcApp" 
	//    (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
	// ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX);      // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;		         // Instância da janela actualmente exibida 
								   // ("hInst" é parâmetro de WinMain e vem 
										 // inicializada daí)
	wcApp.lpszClassName = szProgName;       // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos;       // Endereço da função de processamento da janela
											// ("TrataEventos" foi declarada no início e
											// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for
											// modificada horizontal ou verticalmente

	wcApp.hIcon = LoadIcon(NULL, IDI_QUESTION);   // "hIcon" = handler do ícon normal
										   // "NULL" = Icon definido no Windows
										   // "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(NULL, IDI_SHIELD); // "hIconSm" = handler do ícon pequeno
										   // "NULL" = Icon definido no Windows
										   // "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);	// "hCursor" = handler do cursor (rato) 
							  // "NULL" = Forma definida no Windows
							  // "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = NULL;			// Classe do menu que a janela pode ter
							  // (NULL = não tem menu)
	wcApp.cbClsExtra = 0;				// Livre, para uso particular
	wcApp.cbWndExtra = 0;				// Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco

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
		TEXT("Exemplo de Janela Principal em C"),// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW,	// Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT,		// Posição x pixels (default=à direita da última)
		CW_USEDEFAULT,		// Posição y pixels (default=abaixo da última)
		CW_USEDEFAULT,		// Largura da janela (em pixels)
		CW_USEDEFAULT,		// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de
						// outra) ou HWND_DESKTOP se a janela for a primeira, 
						// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da instância do programa actual ("hInst" é 
						// passado num dos parâmetros de WinMain()
		0);				// Não há parâmetros adicionais para a janela
	  // ============================================================================
	  // 4. Mostrar a janela
	  // ============================================================================
	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por 
					  // "CreateWindow"; "nCmdShow"= modo de exibição (p.e. 
					  // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd);		// Refrescar a janela (Windows envia à janela uma 
					  // mensagem para pintar, mostrar dados, (refrescar)… 
	// ============================================================================
	// 5. Loop de Mensagens
	// ============================================================================
	// O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
	// espera até que GetMessage(...) possa ler "a mensagem seguinte"	
	// Parâmetros de "getMessage":
	// 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
	//   início de WinMain()):
	//			HWND hwnd		handler da janela a que se destina a mensagem
	//			UINT message		Identificador da mensagem
	//			WPARAM wParam		Parâmetro, p.e. código da tecla premida
	//			LPARAM lParam		Parâmetro, p.e. se ALT também estava premida
	//			DWORD time		Hora a que a mensagem foi enviada pelo Windows
	//			POINT pt		Localização do mouse (x, y) 
	// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
	//   receber as mensagens para todas as
	// janelas pertencentes à thread actual)
	// 3)Código limite inferior das mensagens que se pretendem receber
	// 4)Código limite superior das mensagens que se pretendem receber

	// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
	// 	  terminando então o loop de recepção de mensagens, e o programa 

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);	// Pré-processamento da mensagem (p.e. obter código 
					   // ASCII da tecla premida)
		DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
					   // aguarda até que a possa reenviar à função de 
					   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam);	// Retorna sempre o parâmetro wParam da estrutura lpMsg
}

// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da
// estrutura "wcApp", feita no início de // WinMain(), se identifique essa função. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas
// no loop "while" da função WinMain()
// Parâmetros:
//		hWnd	O handler da janela, obtido no CreateWindow()
//		messg	Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
//		wParam	O parâmetro wParam da estrutura messg (a mensagem)
//		lParam	O parâmetro lParam desta mesma estrutura
//
// NOTA:Estes parâmetros estão aqui acessíveis o que simplifica o acesso aos seus valores
//
// A função EndProc é sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar.
// Estas mensagens são identificadas por constantes (p.e. 
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
		//dados.terminar = 1;
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		hdc = GetDC(hWnd);		//a função GetDC recupera um identificador para um contexto de dispositivo (DC) ...
		GetClientRect(hWnd, &rect);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		rect.left = xPos;
		rect.top = yPos;
		DrawText(hdc, &c, 1, &rect, DT_SINGLELINE | DT_NOCLIP);
		ReleaseDC(hWnd, hdc);
		break;
	case WM_RBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		hdc = GetDC(hWnd);		//a função GetDC recupera um identificador para um contexto de dispositivo (DC) ...
		GetClientRect(hWnd, &rect);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		rect.left = xPos;
		rect.top = yPos;
		DrawText(hdc, &mensagem, _tcslen(mensagem), &rect, DT_SINGLELINE | DT_NOCLIP);
		ReleaseDC(hWnd, hdc);
		break;


	case WM_CHAR:	//apanhar  teclado
		c = (wchar_t)wParam;
		break;

	case WM_DESTROY:	//Destruir janela
		PostQuitMessage(0);
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// não é efectuado nenhum processamento, apenas se segue o "default" do Windows
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;  // break tecnicamente desnecessário por causa do return
	}
	return(0);
}
