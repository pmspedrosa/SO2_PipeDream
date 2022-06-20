#include "utils.h"

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



TCHAR comandosParaString(TCHAR** arraycmd, int numargs) {
	TCHAR str[MAX] = _T("");
	TCHAR tk[MAX] = _T(" ");
	TCHAR tk_fim[MAX] = _T("\0");

	_tcscat_s(str, MAX, arraycmd[0]);
	if (numargs > 0) {
		for (int i = 1; i <= numargs; i++)
		{
			_tcscat_s(str, MAX, tk);
			_tcscat_s(str, MAX,arraycmd[i]);
		}
	}
	_tcscat_s(str, MAX, tk_fim);

	return str;
}


int contaEspaços(TCHAR *msg){
	TCHAR ch = msg[0];
	int i = 0, cont=0;

	while (ch != '\0') {
		ch = msg[i];
		if (isspace(ch))
			cont++;
		i++;
	}
	return cont;
}