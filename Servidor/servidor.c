#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 256
#define CHAVE_REGISTRY_NOME TEXT("SOFTWARE\\SO2_PIPE_DREAM")
#define REGISTRY_TAM_H TEXT("TAM_HORIZONTAL")
#define REGISTRY_TAM_V TEXT("TAM_VERTICAL")
#define REGISTRY_TEMPO TEXT("TEMPO_AGUA")
#define TAM_H_OMISSAO 10L
#define TAM_V_OMISSAO 7L
#define TEMPO_AGUA_OMISSAO 10L

DWORD carregaValorConfig(TCHAR valorString[], HKEY hChaveRegistry, TCHAR nomeValorRegistry[], DWORD valorOmissao, DWORD* varGuardar) {
	DWORD valorNum = 0;
	DWORD sizeValor;
	if(valorString != NULL){	
		valorNum = atoi(valorString);
	}
	if (valorNum < 3 || valorNum > 20) {
		sizeValor = sizeof(valorNum);
		if (RegQueryValueEx(hChaveRegistry, nomeValorRegistry, NULL, NULL, &valorNum, &sizeValor) == ERROR_SUCCESS) {
			*varGuardar = valorNum;
			return 1;
		}
		else {
			*varGuardar = valorOmissao;
			RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(DWORD));
			return 2;
		}
	}
	else {
		*varGuardar = valorNum;
		RegSetValueEx(hChaveRegistry, nomeValorRegistry, 0, REG_DWORD, varGuardar, sizeof(DWORD));
		return 3;
	}


}

int _tmain(int argc, LPTSTR argv[]) {
	HKEY hChaveRegistry;
	DWORD respostaRegistry;
	DWORD longGenerico1, longGenerico2;

	//não sei como planeamos guardar esta informação. Provavelmente uma estrutura de dados
	DWORD tamHorizontal, tamVertical, tempoInicioAgua;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	if (RegCreateKeyEx(HKEY_CURRENT_USER, CHAVE_REGISTRY_NOME, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hChaveRegistry, &respostaRegistry) != ERROR_SUCCESS) {
		DWORD error = GetLastError();
		_tprintf(TEXT("Erro a abrir a chave [%d]\n"), hChaveRegistry);
		return -1;
	}
	if (argc >= 4) {
		carregaValorConfig(argv[1], hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal);
		carregaValorConfig(argv[2], hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical);
		carregaValorConfig(argv[3], hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &tempoInicioAgua);
	}
	else {
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_H, TAM_H_OMISSAO, &tamHorizontal);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TAM_V, TAM_V_OMISSAO, &tamVertical);
		carregaValorConfig(NULL, hChaveRegistry, REGISTRY_TEMPO, TEMPO_AGUA_OMISSAO, &tempoInicioAgua);
	}

	_tprintf(TEXT("tam_h -> %d  \ntam_v -> %ld\ntempo -> %ld\n"), tamHorizontal, tamVertical, tempoInicioAgua);


	RegCloseKey(hChaveRegistry);
	return 0;
}